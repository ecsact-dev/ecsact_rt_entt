#pragma once

#include <cassert>
#include <vector>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <tuple>
#include <execution>
#include <span>
#include <boost/mp11.hpp>
#include <ecsact/runtime.hh>
#include <ecsact/runtime/common.h>
#include <ecsact/runtime/core.h>
#include <ecsact/lib.hh>
#include <entt/entt.hpp>

#include "runtime-util/runtime-util.hh"

#include "system_execution_context.hh"
#include "execution_events_collector.hh"
#include "registry_info.hh"
#include "event_markers.hh"

namespace ecsact::entt {
	template<::ecsact::package Package>
	class runtime {
		/**
		 * Checks if type T is listd as one of the actions in the ecact package.
		 * @returns `true` if T is a component belonging to `package`, `false` 
		 *          otherwise.
		 */
		template<typename T>
		static constexpr bool is_action() {
			using boost::mp11::mp_bind_front;
			using boost::mp11::mp_transform_q;
			using boost::mp11::mp_any;
			using boost::mp11::mp_apply;

			return mp_apply<mp_any, mp_transform_q<
				mp_bind_front<std::is_same, std::remove_cvref_t<T>>,
				typename Package::actions
			>>::value;
		}

		using registry_info = ecsact_entt_rt::registry_info<Package>;

		using registries_map_t = std::unordered_map
			< ::ecsact::registry_id
			, registry_info
			>;

		using actions_span_t = std::span<ecsact_action, std::dynamic_extent>;

		::ecsact::registry_id _last_registry_id{};
		registries_map_t _registries;

	public:
		using system_execution_context =
			ecsact_entt_rt::system_execution_context<Package>;
		using execution_events_collector =
			ecsact_entt_rt::execution_events_collector;
		using registry_type = ::entt::registry;
		using entt_entity_type = typename registry_type::entity_type;
		using package = Package;

		::ecsact::registry_id create_registry
			( const char* registry_name
			)
		{
			using boost::mp11::mp_for_each;

			// Using the index of _registries as an ID
			const auto reg_id = static_cast<::ecsact::registry_id>(
				static_cast<int>(_last_registry_id) + 1
			);

			_last_registry_id = reg_id;

			auto itr = _registries.emplace_hint(
				_registries.end(),
				std::piecewise_construct,
				std::forward_as_tuple(reg_id),
				std::forward_as_tuple()
			);

			registry_info& info = itr->second;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				info.registry.storage<C>();
			});

			mp_for_each<typename package::system_addables>([&]<typename C>(C) {
				info.registry.storage<component_added<C>>();
			});

			mp_for_each<typename package::system_writables>([&]<typename C>(C) {
				using detail::beforechange_storage;

				info.registry.storage<beforechange_storage<C>>();
				info.registry.storage<component_changed<C>>();
			});

			mp_for_each<typename package::system_removables>([&]<typename C>(C) {
				using detail::temp_storage;

				info.registry.storage<temp_storage<C>>();
				info.registry.storage<component_removed<C>>();
			});

			return reg_id;
		}

		void destroy_registry
			( ::ecsact::registry_id reg_id
			)
		{
			_registries.erase(reg_id);
		}

		void clear_registry
			( ::ecsact::registry_id reg_id
			)
		{
			using boost::mp11::mp_for_each;

			auto& info = _registries.at(reg_id);

			info.registry.clear();
			info.entities_map.clear();
			info._ecsact_entity_ids.clear();
			info.last_entity_id = {};
		}

		::ecsact::entity_id create_entity
			( ::ecsact::registry_id reg_id
			)
		{
			std::mutex mutex;
			auto& info = _registries.at(reg_id);
			info.mutex = mutex;
			auto new_entity_id = info.create_entity().ecsact_entity_id;
			info.mutex = std::nullopt;
			return new_entity_id;
		}

		void ensure_entity
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			if(!info.entities_map.contains(entity_id)) {
				std::mutex mutex;
				info.mutex = mutex;
				info.create_entity(entity_id);
				info.mutex = std::nullopt;
			}
		}

		bool entity_exists
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			return info.entities_map.contains(entity_id);
		}

		void destroy_entity
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			using boost::mp11::mp_for_each;

			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			info.registry.destroy(entt_entity_id);
			info.entities_map.erase(entity_id);
		}

		int count_entities
			( ::ecsact::registry_id reg_id
			)
		{
			auto& info = _registries.at(reg_id);
			return static_cast<int>(info.registry.size());
		}

		std::vector<ecsact::entity_id> get_entities
			( ::ecsact::registry_id reg_id
			)
		{
			auto& info = _registries.at(reg_id);
			std::vector<ecsact::entity_id> result;
			for(auto& entry: info.entities_map) {
				result.push_back(entry.first);
			}

			return result;
		}

		void get_entities
			( ::ecsact::registry_id  reg_id
			, int                    max_entities_count
			, ::ecsact::entity_id*   out_entities
			, int*                   out_entities_count
			)
		{
			auto& info = _registries.at(reg_id);

			int entities_count = static_cast<int>(info.entities_map.size());
			max_entities_count = std::min(entities_count, max_entities_count);

			auto itr = info.entities_map.begin();
			for(int i=0; max_entities_count > i; ++i) {
				if(itr == info.entities_map.end()) break;

				out_entities[i] = itr->first;
				++itr;
			}

			if(out_entities_count != nullptr) {
				*out_entities_count = entities_count;
			}
		}

		template<typename C>
		void add_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			, const C&               component_data
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			if constexpr(std::is_empty_v<C>) {
				info.add_component<C>(entt_entity_id);
			} else {
				info.add_component<C>(entt_entity_id, component_data);
			}
		}

		template<typename ComponentT>
		void add_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			add_component<ComponentT>(reg_id, entity_id, ComponentT{});
		}

		void add_component
			( ::ecsact::registry_id   reg_id
			, ::ecsact::entity_id     entity_id
			, ::ecsact::component_id  component_id
			, const void*             component_data
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					if constexpr(std::is_empty_v<C>) {
						add_component<C>(reg_id, entity_id);
					} else {
						add_component<C>(
							reg_id,
							entity_id,
							*static_cast<const C*>(component_data)
						);
					}
				}
			});
		}

		template<typename ComponentT>
		bool has_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			return info.registry.all_of<ComponentT>(entt_entity_id);
		}

		bool has_component
			( ::ecsact::registry_id   reg_id
			, ::ecsact::entity_id     entity_id
			, ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			bool result = false;
			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					result = has_component<C>(reg_id, entity_id);
				}
			});
			return result;
		}

		template<typename ComponentT>
		const ComponentT& get_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			return info.registry.get<ComponentT>(entt_entity_id);
		}

		const void* get_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			, ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			const void* component_data = nullptr;
			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					if constexpr(std::is_empty_v<C>) {
						static C c{};
						component_data = &c;
					} else {
						const C& comp_ref = get_component<C>(reg_id, entity_id);
						component_data = &comp_ref;
					}
				}
			});
			return component_data;
		}

		template<typename ComponentT>
		void update_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			, const ComponentT&      component_data
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);
	
			auto& component = info.registry.get<ComponentT>(entt_entity_id);
			component = component_data;
		}

		void update_component
			( ::ecsact::registry_id   reg_id
			, ::ecsact::entity_id     entity_id
			, ::ecsact::component_id  component_id
			, const void*             component_data
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					if constexpr(!std::is_empty_v<C>) {
						update_component<C>(
							reg_id,
							entity_id,
							*static_cast<const C*>(component_data)
						);
					}
				}
			});
		}

		template<typename C>
		void remove_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			info.remove_component<C>(entt_entity_id);
		}

		void remove_component
			( ::ecsact::registry_id   reg_id
			, ::ecsact::entity_id     entity_id
			, ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					remove_component<C>(reg_id, entity_id);
				}
			});
		}

	private:

		template<typename SystemT>
		void _apply_pending_adds
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;
			using boost::mp11::mp_unique;
			using boost::mp11::mp_flatten;
			using boost::mp11::mp_push_back;
			using ecsact::entt::detail::pending_add;

			// using flattened_generates = typename SystmT::generates

			using addables = mp_unique<mp_flatten<mp_push_back<
				typename SystemT::generates,
				typename SystemT::adds
			>>>;

			mp_for_each<addables>([&]<typename C>(C) {
				using boost::mp11::mp_apply;
				using boost::mp11::mp_bind_front;
				using boost::mp11::mp_transform_q;
				using boost::mp11::mp_any;

				// Making sure all the components in `addables` list are indeed package
				// components. If this assertion fails there was an error in creating
				// the `addables` alias.
				static_assert(
					mp_apply<mp_any, mp_transform_q<
						mp_bind_front<std::is_same, std::remove_cvref_t<C>>,
						typename Package::components
					>>::value
				);

				auto view = info.registry.view<pending_add<C>>();
				if constexpr(std::is_empty_v<C>) {
					view.each([&](auto entity) {
						info.add_component<C>(entity);
					});
				} else {
					view.each([&](auto entity, auto& component) {
						info.add_component<C>(entity, component.value);
					});
				}

				info.registry.clear<pending_add<C>>();
			});
		}

		template<typename SystemT>
		void _apply_pending_removes
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;
			using ecsact::entt::detail::pending_remove;

			mp_for_each<typename SystemT::removes>([&]<typename C>(C) {
				auto view = info.registry.view<pending_remove<C>>();
				view.each([&](auto entity) {
					info.remove_component<C>(entity);
				});

				info.registry.clear<pending_remove<C>>();
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial_removes_only
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			, const actions_span_t&             actions
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename SystemT::removes>([&]<typename C>(C) {
				info.registry.clear<C>();
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial_default_itr
			( registry_info&                    info
			, entt_entity_type                  entity
			, ecsact_system_execution_context*  parent
			, const void*                       action
			, const actions_span_t&             actions
			)
		{
			using boost::mp11::mp_for_each;

			system_execution_context ctx{
				.info = info,
				.entity = entity,
				.parent = parent,
				.action = action,
			};

			mp_for_each<typename SystemT::removes>([&]<typename C>(C) {
				ctx.remove<C>();
			});
			mp_for_each<typename SystemT::adds>([&]<typename C>(C) {
				ctx.add<C>(C{});
			});

			mp_for_each<ChildSystemsListT>([&]<typename SystemPair>(SystemPair) {
				using boost::mp11::mp_first;
				using boost::mp11::mp_second;
				using ChildSystemT = mp_first<SystemPair>;
				using GrandChildSystemsListT = mp_second<SystemPair>;

				_execute_system<ChildSystemT, GrandChildSystemsListT>(
					info,
					ctx.cptr(),
					actions
				);
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial_default
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			, const actions_span_t&             actions
			)
		{
			using boost::mp11::mp_empty;
			using std::execution::par_unseq;
			using std::execution::seq;

#ifndef NDEBUG
			[[maybe_unused]] auto system_name = typeid(SystemT).name();
#endif

			auto view = ecsact_entt_view(
				std::type_identity<SystemT>{},
				info.registry
			);

			constexpr bool can_exec_parallel =
				mp_empty<ChildSystemsListT>::value &&
				mp_empty<SystemT::adds>::value &&
				mp_empty<SystemT::removes>::value &&
				mp_empty<SystemT::generates>::value;

			if constexpr(can_exec_parallel) {
				// TODO(zaucy): Make this par_unseq
				std::for_each(par_unseq, view.begin(), view.end(), [&](auto entity) {
					_execute_system_trivial_default_itr<SystemT, ChildSystemsListT>(
						info,
						entity,
						parent,
						action,
						actions
					);
				});
			} else {
				std::for_each(seq, view.begin(), view.end(), [&](auto entity) {
					_execute_system_trivial_default_itr<SystemT, ChildSystemsListT>(
						info,
						entity,
						parent,
						action,
						actions
					);
				});
			}
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			, const actions_span_t&             actions
			)
		{
			using boost::mp11::mp_for_each;
			using boost::mp11::mp_empty;
			using boost::mp11::mp_size;

			static_assert(SystemT::has_trivial_impl);

			using excludes_list = typename SystemT::excludes;
			using includes_list = typename SystemT::includes;
			using removes_list = typename SystemT::removes;
			using adds_list = typename SystemT::adds;

			// Check if we are doing a blanket remove for an optimized system
			// implementation.
			constexpr bool is_removes_only =
				mp_empty<excludes_list>::value && 
				mp_empty<adds_list>::value &&
				mp_empty<includes_list>::value &&
				(mp_size<removes_list>::value == 1);

			if constexpr(is_removes_only) {
				_execute_system_trivial_removes_only<SystemT, ChildSystemsListT>(
					info,
					parent,
					action,
					actions
				);
			} else {
				_execute_system_trivial_default<SystemT, ChildSystemsListT>(
					info,
					parent,
					action,
					actions
				);
			}
		}

		template<typename SystemT>
		void _prepare_check_component_changes
			( system_execution_context&  ctx
			)
		{
			using boost::mp11::mp_empty;
			using boost::mp11::mp_size;
			using boost::mp11::mp_for_each;

			if constexpr(!mp_empty<typename SystemT::writables>::value) {
				ctx.writables.reserve(mp_size<typename SystemT::writables>::value);

				mp_for_each<typename SystemT::writables>([&]<typename C>(C) {
					if constexpr(C::transient) return;

					if(!ctx.info.registry.all_of<component_changed<C>>(ctx.entity)) {
						ctx.writables.emplace(C::id);
					}
				});
			}
		}

		template<typename SystemT>
		void _check_component_changes
			( system_execution_context&  ctx
			, auto&                      component_source
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename SystemT::writables>([&]<typename C>(C) {
				using ecsact::entt::detail::beforechange_storage;
				if(!ctx.writables.contains(C::id)) return;
				if(!ctx.info.registry.all_of<beforechange_storage<C>>(ctx.entity)) {
					return;
				}

				const auto component_name = typeid(C).name();

				auto& before = ctx.info.registry.get<beforechange_storage<C>>(
					ctx.entity
				);

				if(before.set) {
					auto& after = component_source.get<C>(ctx.entity);

					if(before.value != after) {
						ctx.info.registry.emplace<component_changed<C>>(ctx.entity);
					}
					before.set = false;
				}
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_user_itr
			( registry_info&                    info
			, entt_entity_type                  entity
			, ecsact_system_execution_context*  parent
			, const void*                       action
			, auto&                             view
			, const actions_span_t&             actions
			)
		{
			using boost::mp11::mp_for_each;

			const auto system_name = typeid(SystemT).name();

			system_execution_context ctx{
				.info = info,
				.entity = entity,
				.parent = parent,
				.action = action,
				.writables{},
			};

			_prepare_check_component_changes<SystemT>(ctx);

			// Execute the user defined system implementation
			SystemT::dynamic_impl(ctx.cpp_ptr());

			_check_component_changes<SystemT>(ctx, view);

			mp_for_each<ChildSystemsListT>([&]<typename SystemPair>(SystemPair) {
				using boost::mp11::mp_first;
				using boost::mp11::mp_second;
				using ChildSystemT = mp_first<SystemPair>;
				using GrandChildSystemsListT = mp_second<SystemPair>;

				_execute_system<ChildSystemT, GrandChildSystemsListT>(
					info,
					ctx.cptr(),
					actions
				);
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_user
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			, const actions_span_t&             actions
			)
		{
			using boost::mp11::mp_empty;
			using std::execution::seq;
			using std::execution::par_unseq;

			static_assert(!SystemT::has_trivial_impl);

			auto view = ecsact_entt_view(
				std::type_identity<SystemT>{},
				info.registry
			);

			constexpr bool can_exec_parallel =
				mp_empty<ChildSystemsListT>::value &&
				mp_empty<SystemT::adds>::value &&
				mp_empty<SystemT::removes>::value;

			if constexpr(can_exec_parallel) {
				// TODO(zaucy): Make this par_unseq
				std::for_each(seq, view.begin(), view.end(), [&](auto entity) {
					_execute_system_user_itr<SystemT, ChildSystemsListT>(
						info,
						entity,
						parent,
						action,
						view,
						actions
					);
				});
			} else {
				std::for_each(seq, view.begin(), view.end(), [&](auto entity) {
					_execute_system_user_itr<SystemT, ChildSystemsListT>(
						info,
						entity,
						parent,
						action,
						view,
						actions
					);
				});
			}
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const actions_span_t&             actions
			)
		{
			if constexpr(is_action<SystemT>()) {
				for(const ecsact_action& action : actions) {
					if(action.action_id == static_cast<ecsact_system_id>(SystemT::id)) {
						if constexpr(SystemT::has_trivial_impl) {
							_execute_system_trivial<SystemT, ChildSystemsListT>(
								info,
								parent,
								static_cast<const SystemT*>(action.action_data),
								actions
							);
						} else {
							_execute_system_user<SystemT, ChildSystemsListT>(
								info,
								parent,
								static_cast<const SystemT*>(action.action_data),
								actions
							);
						}
					}
				}
			} else {
				if constexpr(SystemT::has_trivial_impl) {
					_execute_system_trivial<SystemT, ChildSystemsListT>(
						info,
						parent,
						nullptr,
						actions
					);
				} else {
					_execute_system_user<SystemT, ChildSystemsListT>(
						info,
						parent,
						nullptr,
						actions
					);
				}
			}

			_apply_pending_removes<SystemT>(info);
			_apply_pending_adds<SystemT>(info);
		}

		void _clear_transients
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				// Transients require no processing, just clear.
				if constexpr(C::transient) {
					info.registry.clear<C>();
				}
			});
		}

		void _sort_components
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(!std::is_empty_v<C> && !C::transient) {
					// Sorting for deterministic order of components when executing
					// systems.
					// TODO(zaucy): This sort is only necessary for components part of a 
					//              system execution hierarchy greater than 1.
					info.registry.sort<C>([](const C& a, const C& b) -> bool {
						return a < b;
					});
				}
			});
		}

		void _trigger_init_component_events
			( registry_info&               info
			, execution_events_collector&  events_collector
			)
		{
			using boost::mp11::mp_for_each;

			if(!events_collector.has_init_callback()) return;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) return;

				::entt::basic_view added_view{
					info.registry.storage<C>(),
					info.registry.storage<component_added<C>>(),
				};
				
				for(entt_entity_type entity : added_view) {
					if constexpr(std::is_empty_v<C>) {
						events_collector.invoke_init_callback<C>(
							info.ecsact_entity_id(entity)
						);
					} else {
						events_collector.invoke_init_callback<C>(
							info.ecsact_entity_id(entity),
							added_view.get<C>(entity)
						);
					}
				}
			});
		}

		void _trigger_update_component_events
			( registry_info&               info
			, execution_events_collector&  events_collector
			)
		{
			using boost::mp11::mp_for_each;
			using detail::beforechange_storage;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(!C::transient && !std::is_empty_v<C>) {
					::entt::basic_view changed_view{
						info.registry.storage<C>(),
						info.registry.storage<beforechange_storage<C>>(),
						info.registry.storage<component_changed<C>>(),
					};
					
					for(entt_entity_type entity : changed_view) {
						auto& before = changed_view.get<beforechange_storage<C>>(entity);
						auto& current = changed_view.get<C>(entity);
						
						if(before.value != current) {
							events_collector.invoke_update_callback<C>(
								info.ecsact_entity_id(entity),
								current
							);
						}
					}
				}
			});
		}

		void _trigger_remove_component_events
			( registry_info&               info
			, execution_events_collector&  events_collector
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) return;

				::entt::basic_view removed_view{
					info.registry.storage<detail::temp_storage<C>>(),
					info.registry.storage<component_removed<C>>(),
				};

				for(entt_entity_type entity : removed_view) {
					if constexpr(std::is_empty_v<C>) {
						events_collector.invoke_remove_callback<C>(
							info.ecsact_entity_id(entity)
						);
					} else {
						events_collector.invoke_remove_callback<C>(
							info.ecsact_entity_id(entity),
							removed_view.get<detail::temp_storage<C>>(entity).value
						);
					}

					info.registry.storage<detail::temp_storage<C>>().remove(entity);
				}
			});
		}

		void _execute_systems
			( registry_info&   info
			, actions_span_t&  actions
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::execution_order>(
				[&]<typename SystemList>(SystemList) {
					using boost::mp11::mp_size;
					using boost::mp11::mp_first;
					using boost::mp11::mp_second;
					using std::execution::par_unseq;

					if constexpr(mp_size<SystemList>::value > 1) {
						mp_for_each<SystemList>([&]<typename SystemPair>(SystemPair) {
							using SystemT = mp_first<SystemPair>;
							using ChildSystemsListT = mp_second<SystemPair>;
							_execute_system<SystemT, ChildSystemsListT>(
								info,
								nullptr,
								actions
							);
						});
					} else {
						using SystemPair = mp_first<SystemList>;
						using SystemT = mp_first<SystemPair>;
						using ChildSystemsListT = mp_second<SystemPair>;
						_execute_system<SystemT, ChildSystemsListT>(
							info,
							nullptr,
							actions
						);
					}
				}
			);
		}

		template<typename C>
			requires(!std::is_empty_v<C>)
		void _pre_exec_add_component
			( registry_info&    info
			, entt_entity_type  entity
			, const C&          component
			)
		{
#ifndef NDEBUG
			{
				const bool already_has_component = info.registry.all_of<C>(entity);
				if(already_has_component) {
					using namespace std::string_literals;
					std::string err_msg = "Entity already has component. ";
					err_msg += "Attempted added component: "s + typeid(C).name();
					throw std::runtime_error(err_msg.c_str());
				}
			}
#endif

			info.add_component<C>(entity, component);
			info.registry.emplace<component_added<C>>(entity);
		}

		template<typename C>
			requires(std::is_empty_v<C>)
		void _pre_exec_add_component
			( registry_info&    info
			, entt_entity_type  entity
			)
		{
#ifndef NDEBUG
			if(info.registry.all_of<C>(entity)) {
				using namespace std::string_literals;
				std::string err_msg = "Entity already has component. ";
				err_msg += "Attempted added component: "s + typeid(C).name();
				throw std::runtime_error(err_msg.c_str());
			}
#endif

			info.add_component<C>(entity);
			info.registry.emplace<component_added<C>>(entity);
		}

		template<typename C>
			requires(!std::is_empty_v<C>)
		void _pre_exec_update_component
			( registry_info&    info
			, entt_entity_type  entity
			, const C&          component
			)
		{
#ifndef NDEBUG
			if(!info.registry.all_of<C>(entity)) {
				using namespace std::string_literals;
				std::string err_msg = "Entity does not have component. ";
				err_msg += "Attempted update on component: "s + typeid(C).name();
				throw std::runtime_error(err_msg.c_str());
			}
#endif

			// No change. Do nothing.
			if(info.registry.get<C>(entity) == component) return;

			info.registry.get<C>(entity) = component;
			if(!info.registry.all_of<component_added<C>>(entity)) {
				if(!info.registry.all_of<component_changed<C>>(entity)) {
					info.registry.emplace<component_changed<C>>(entity);
				}
			}
		}

		template<typename C>
		void _pre_exec_remove_component
			( registry_info&    info
			, entt_entity_type  entity
			)
		{
			using detail::temp_storage;

			if(info.registry.all_of<component_added<C>>(entity)) {
				info.registry.remove<component_added<C>>(entity);
			}
			if constexpr(!std::is_empty_v<C>) {
				auto& temp = info.registry.storage<temp_storage<C>>();

				// Store current value of component for the before_remove event later
				if(temp.contains(entity)) {
					temp.get(entity).value = info.registry.get<C>(entity);
				} else {
					temp.emplace(entity, info.registry.get<C>(entity));
				}
			}

			info.remove_component<C>(entity);
			if(!info.registry.all_of<component_removed<C>>(entity)) {
				info.registry.emplace<component_removed<C>>(entity);
			}
			info.registry.remove<component_changed<C>>(entity);
			info.registry.remove<component_added<C>>(entity);
		}

		void _apply_execution_options
			( const ecsact_execution_options&  options
			, registry_info&                   info
			)
		{
			using boost::mp11::mp_for_each;

			for(int i=0; options.add_components_length > i; ++i) {
				const ecsact_entity_id& entity = options.add_components_entities[i];
				const ecsact_component& comp = options.add_components[i];

				mp_for_each<typename package::components>([&]<typename C>(C) {
					if constexpr(C::transient) return;

					if(comp.component_id == static_cast<ecsact_component_id>(C::id)) {
						if constexpr(std::is_empty_v<C>) {
							_pre_exec_add_component<C>(
								info,
								info.entities_map.at(static_cast<::ecsact::entity_id>(entity))
							);
						} else {
							_pre_exec_add_component<C>(
								info,
								info.entities_map.at(static_cast<::ecsact::entity_id>(entity)),
								*static_cast<const C*>(comp.component_data)
							);
						}
					}
				});
			}

			for(int i=0; options.update_components_length > i; ++i) {
				const ecsact_entity_id& entity = options.update_components_entities[i];
				const ecsact_component& comp = options.update_components[i];

				mp_for_each<typename package::components>([&]<typename C>(C) {
					if constexpr(C::transient) return;

					if(comp.component_id == static_cast<ecsact_component_id>(C::id)) {
						if constexpr(!std::is_empty_v<C>) {
							_pre_exec_update_component<C>(
								info,
								info.entities_map.at(static_cast<::ecsact::entity_id>(entity)),
								*static_cast<const C*>(comp.component_data)
							);
						} else {
							assert(!std::is_empty_v<C>);
						}
					}
				});
			}

			for(int i=0; options.remove_components_length > i; ++i) {
				const ecsact_entity_id& entity = options.update_components_entities[i];
				ecsact_component_id component_id = options.remove_components[i];

				mp_for_each<typename package::components>([&]<typename C>(C) {
					if constexpr(C::transient) return;

					if(component_id == static_cast<ecsact_component_id>(C::id)) {
						_pre_exec_remove_component<C>(
							info,
							info.entities_map.at(static_cast<::ecsact::entity_id>(entity))
						);
					}
				});
			}
		}

		void _clear_event_markers
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) return;

				info.registry.clear<component_added<C>>();
			});

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) return;

				info.registry.storage<component_changed<C>>().clear();
			});

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) return;

				info.registry.clear<component_removed<C>>();
			});
		}

	public:
		void execute_systems
			( ::ecsact::registry_id                      reg_id
			, int                                        execution_count
			, const ecsact_execution_options*            execution_options_list
			, std::optional<execution_events_collector>  events_collector
			)
		{
			std::mutex mutex;
			auto& info = _registries.at(reg_id);
			info.mutex = std::ref(mutex);

			for(int n=0; execution_count > n; ++n) {
				actions_span_t actions;
				if(execution_options_list != nullptr) {
					_apply_execution_options(execution_options_list[n], info);
					if(execution_options_list->actions_length > 0) {
						actions = std::span(
							execution_options_list->actions,
							execution_options_list->actions_length
						);
					}
				}
				// _sort_components(info);
				_execute_systems(info, actions);
				_clear_transients(info);
			}
			if(events_collector) {
				_trigger_init_component_events(info, *events_collector);
				_trigger_update_component_events(info, *events_collector);
				_trigger_remove_component_events(info, *events_collector);
			}
			_clear_event_markers(info);

			info.mutex = std::nullopt;
		}

	};

}
