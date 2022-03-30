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
#include <boost/mp11.hpp>
#include <ecsact/runtime.hh>
#include <ecsact/runtime/common.h>
#include <ecsact/runtime/core.h>
#include <ecsact/lib.hh>
#include <entt/entt.hpp>

#include "runtime-util/runtime-util.hh"

#include "system_execution_context.hh"
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

		::ecsact::registry_id _last_registry_id{};
		registries_map_t _registries;

	public:
		using system_execution_context =
			ecsact_entt_rt::system_execution_context<Package>;

	private:
		template<typename ComponentT>
		void _invoke_add_callbacks
			( registry_info&       info
			, ::ecsact::entity_id  entity
			, const ComponentT&    component
			)
		{
			if(info.add_component_callbacks.contains(ComponentT::id)) {
				info.add_component_callbacks.at(ComponentT::id)(
					static_cast<ecsact_entity_id>(entity),
					static_cast<const void*>(&component)
				);
			}

			info.add_any_component_callbacks(
				static_cast<ecsact_entity_id>(entity),
				static_cast<ecsact_component_id>(ComponentT::id),
				static_cast<const void*>(&component)
			);
		}

		template<typename ComponentT>
		void _invoke_update_callbacks
			( registry_info&       info
			, ::ecsact::entity_id  entity
			, const ComponentT&    component
			)
		{
			if(info.update_component_callbacks.contains(ComponentT::id)) {
				info.update_component_callbacks.at(ComponentT::id)(
					static_cast<ecsact_entity_id>(entity),
					static_cast<const void*>(&component)
				);
			}

			info.update_any_component_callbacks(
				static_cast<ecsact_entity_id>(entity),
				static_cast<ecsact_component_id>(ComponentT::id),
				static_cast<const void*>(&component)
			);
		}

		template<typename C>
		void _invoke_before_remove_callbacks
			( registry_info&                          info
			, ::ecsact::entity_id                     entity_id
			, typename ::entt::registry::entity_type  entt_entity_id
			, auto&&                                  component_source
			)
		{
			if(info.before_remove_component_callbacks.contains(C::id)) {
				if constexpr(std::is_empty_v<C>) {
					C c{};
					info.before_remove_component_callbacks.at(C::id)(
						static_cast<ecsact_entity_id>(entity_id),
						&c
					);
				} else {
					auto& component = component_source.get<detail::temp_storage<C>>(
						entt_entity_id
					).value;
					info.before_remove_component_callbacks.at(C::id)(
						static_cast<ecsact_entity_id>(entity_id),
						static_cast<const void*>(&component)
					);
				}
			}

			if(!info.before_remove_any_component_callbacks.empty()) {
				if constexpr(std::is_empty_v<C>) {
					C c{};
					info.before_remove_any_component_callbacks(
						static_cast<ecsact_entity_id>(entity_id),
						static_cast<ecsact_component_id>(C::id),
						&c
					);
				} else {
					auto& component = component_source.get<detail::temp_storage<C>>(
						entt_entity_id
					).value;
					info.before_remove_any_component_callbacks(
						static_cast<ecsact_entity_id>(entity_id),
						static_cast<ecsact_component_id>(C::id),
						static_cast<const void*>(&component)
					);
				}
			}
		}

		template<typename ComponentT>
		void _invoke_before_remove_callbacks
			( registry_info&                          info
			, typename ::entt::registry::entity_type  entt_entity_id
			, auto&&                                  component_source
			)
		{
			auto ecsact_entity_id = info.ecsact_entity_id(entt_entity_id);
			_invoke_before_remove_callbacks<ComponentT>(
				info,
				ecsact_entity_id,
				entt_entity_id,
				info.registry
			);
		}

		template<typename ComponentT>
		void _invoke_before_remove_callbacks
			( registry_info&       info
			, ::ecsact::entity_id  ecsact_entity_id
			)
		{
			auto entt_entity_id = info.entities_map.at(ecsact_entity_id);
			_invoke_before_remove_callbacks<ComponentT>(
				info,
				ecsact_entity_id,
				entt_entity_id,
				info.registry
			);
		}

		bool _has_before_remove_callbacks
			( registry_info& info
			)
		{
			return !info.before_remove_component_callbacks.empty()
			    || !info.before_remove_any_component_callbacks.empty();
		}

		bool _has_after_remove_callbacks
			( registry_info& info
			)
		{
			return !info.after_remove_component_callbacks.empty()
			    || !info.after_remove_any_component_callbacks.empty();
		}

		bool _has_any_remove_callbacks
			( registry_info& info
			)
		{
			return _has_before_remove_callbacks(info)
			    || _has_after_remove_callbacks(info);
		}

		template<typename ComponentT>
		void _invoke_after_remove_callbacks
			( registry_info&       info
			, ::ecsact::entity_id  entity
			)
		{
			if(info.after_remove_component_callbacks.contains(ComponentT::id)) {
				info.after_remove_component_callbacks.at(ComponentT::id)(
					static_cast<ecsact_entity_id>(entity)
				);
			}

			info.after_remove_any_component_callbacks(
				static_cast<ecsact_entity_id>(entity),
				static_cast<ecsact_component_id>(ComponentT::id)
			);
		}

	public:
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
				using detail::temp_storage;

				static_cast<void>(info.registry.storage<C>());

				if constexpr(!C::transient) {
					static_cast<void>(info.registry.storage<temp_storage<C>>());
					static_cast<void>(info.registry.storage<component_added<C>>());
					static_cast<void>(info.registry.storage<component_removed<C>>());
				}
			});

			mp_for_each<typename package::system_writables>([&]<typename C>(C) {
				using detail::beforechange_storage;

				static_cast<void>(info.registry.storage<beforechange_storage<C>>());
				static_cast<void>(info.registry.storage<component_changed<C>>());
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
			std::vector<std::function<void(registry_info&)>> after_remove_fns;

			if(_has_any_remove_callbacks(info)) {
				for(auto&& [entity, entt_entity_id] : info.entities_map) {
					mp_for_each<typename package::components>([&]<typename C>(C) {
						if(!info.registry.all_of<C>(entt_entity_id)) return;
						_invoke_before_remove_callbacks<C>(info, entity);
						if(_has_after_remove_callbacks(info)) {
							after_remove_fns.emplace_back([this, entity](auto& info) {
								_invoke_after_remove_callbacks<C>(info, entity);
							});
						}
					});
				}
			}

			info.registry.clear();
			info.entities_map.clear();
			info._ecsact_entity_ids.clear();
			info.last_entity_id = {};
			mp_for_each<typename package::actions>([&]<typename A>(A) {
				std::get<std::vector<A>>(info.actions).clear();
			});

			for(auto& fn : after_remove_fns) {
				fn(info);
			}
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
			std::vector<std::function<void(registry_info&)>> after_remove_fns;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(!info.registry.all_of<C>(entt_entity_id)) return;
				_invoke_before_remove_callbacks<C>(info, entity_id);
				after_remove_fns.emplace_back([this, entity_id](auto& info) {
					_invoke_after_remove_callbacks<C>(info, entity_id);
				});
			});

			info.registry.destroy(entt_entity_id);
			info.entities_map.erase(entity_id);

			for(auto& fn : after_remove_fns) {
				fn(info);
			}
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
				info.registry.emplace<C>(entt_entity_id);
				_invoke_add_callbacks<C>(info, entity_id, C{});
			} else {
				auto& component = info.registry.emplace<C>(
					entt_entity_id,
					component_data
				);

				_invoke_add_callbacks<C>(info, entity_id, component);
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
			_invoke_update_callbacks<ComponentT>(info, entity_id, component);
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

			info.registry.remove<C>(entt_entity_id);
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

		template<typename C>
		void on_add_component
			( ::ecsact::registry_id          reg_id
			, ecsact_add_component_callback  callback
			, void*                          callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_component_callbacks[C::id].add(
				callback,
				callback_user_data
			);
		}

		void on_add_component
			( ::ecsact::registry_id          reg_id
			, ::ecsact::component_id         component_id
			, ecsact_add_component_callback  callback
			, void*                          callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_component_callbacks[component_id].add(
				callback,
				callback_user_data
			);
		}

		void off_add_component
			( ::ecsact::registry_id          reg_id
			, ::ecsact::component_id         component_id
			, ecsact_add_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_component_callbacks[component_id].remove(callback);
		}

		template<typename ComponentT>
		void on_update_component
			( ::ecsact::registry_id             reg_id
			, ecsact_update_component_callback  callback
			, void*                             callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_component_callbacks[ComponentT::id].add(
				callback,
				callback_user_data
			);
		}

		void on_update_component
			( ::ecsact::registry_id             reg_id
			, ::ecsact::component_id            component_id
			, ecsact_update_component_callback  callback
			, void*                             callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_component_callbacks[component_id].add(
				callback,
				callback_user_data
			);
		}

		void off_update_component
			( ::ecsact::registry_id             reg_id
			, ::ecsact::component_id            component_id
			, ecsact_update_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_component_callbacks[component_id].remove(callback);
		}

		template<typename ComponentT>
		void on_before_remove_component
			( ::ecsact::registry_id                    reg_id
			, ecsact_before_remove_component_callback  callback
			, void*                                    callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_component_callbacks[ComponentT::id].add(
				callback,
				callback_user_data
			);
		}

		void on_before_remove_component
			( ::ecsact::registry_id                    reg_id
			, ::ecsact::component_id                   component_id
			, ecsact_before_remove_component_callback  callback
			, void*                                    callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_component_callbacks[component_id].add(
				callback,
				callback_user_data
			);
		}

		void off_before_remove_component
			( ::ecsact::registry_id                    reg_id
			, ::ecsact::component_id                   component_id
			, ecsact_before_remove_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_component_callbacks[component_id].remove(callback);
		}

		template<typename ComponentT>
		void on_after_remove_component
			( ::ecsact::registry_id                   reg_id
			, ecsact_after_remove_component_callback  callback
			, void*                                   callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_component_callbacks[ComponentT::id].add(
				callback,
				callback_user_data
			);
		}

		void on_after_remove_component
			( ::ecsact::registry_id                   reg_id
			, ::ecsact::component_id                  component_id
			, ecsact_after_remove_component_callback  callback
			, void*                                   callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_component_callbacks[component_id].add(
				callback,
				callback_user_data
			);
		}

		void off_after_remove_component
			( ::ecsact::registry_id                   reg_id
			, ::ecsact::component_id                  component_id
			, ecsact_after_remove_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_component_callbacks[component_id].remove(callback);
		}

		void on_add_any_component
			( ::ecsact::registry_id              reg_id
			, ecsact_add_any_component_callback  callback
			, void*                              callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_any_component_callbacks.add(
				callback,
				callback_user_data
			);
		}

		void off_add_any_component
			( ::ecsact::registry_id              reg_id
			, ecsact_add_any_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_any_component_callbacks.remove(callback);
		}

		void on_update_any_component
			( ::ecsact::registry_id                 reg_id
			, ecsact_update_any_component_callback  callback
			, void*                                 callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_any_component_callbacks.add(
				callback,
				callback_user_data
			);
		}

		void off_update_any_component
			( ::ecsact::registry_id                 reg_id
			, ecsact_update_any_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_any_component_callbacks.remove(callback);
		}

		void on_before_remove_any_component
			( ::ecsact::registry_id                        reg_id
			, ecsact_before_remove_any_component_callback  callback
			, void*                                        callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_any_component_callbacks.add(
				callback,
				callback_user_data
			);
		}

		void off_before_remove_any_component
			( ::ecsact::registry_id                        reg_id
			, ecsact_before_remove_any_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_any_component_callbacks.remove(callback);
		}

		void on_after_remove_any_component
			( ::ecsact::registry_id                       reg_id
			, ecsact_after_remove_any_component_callback  callback
			, void*                                       callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_any_component_callbacks.add(
				callback,
				callback_user_data
			);
		}

		void off_after_remove_any_component
			( ::ecsact::registry_id                       reg_id
			, ecsact_after_remove_any_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_any_component_callbacks.remove(callback);
		}

		template<typename ActionT>
		void push_action
			( ::ecsact::registry_id  reg_id
			, const ActionT&         action
			)
		{
			auto& info = _registries.at(reg_id);
			std::get<std::vector<ActionT>>(info.actions).push_back(action);
		}

		void push_action
			( ::ecsact::registry_id  reg_id
			, ::ecsact::action_id    system_id
			, const void*            action_data
			)
		{
			using boost::mp11::mp_for_each;
			using boost::mp11::mp_identity;
			using boost::mp11::mp_transform;

			mp_for_each<mp_transform<mp_identity, typename package::actions>>(
				[&]<typename A>(mp_identity<A>) {
					if(A::id == system_id) {
						if constexpr(std::is_empty_v<A>) {
							push_action<A>(reg_id, A{});
						} else {
							push_action<A>(reg_id, *static_cast<const A*>(action_data));
						}
					}
				}
			);
		}

	private:
		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial_removes_only
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename SystemT::removes>([&]<typename C>(C) {
				info.registry.clear<C>();
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial_default
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			)
		{
			using boost::mp11::mp_for_each;
			using std::execution::seq;

#ifndef NDEBUG
			[[maybe_unused]] auto system_name = typeid(SystemT).name();
#endif

			auto view = ecsact_entt_view(
				std::type_identity<SystemT>{},
				info.registry
			);

			std::for_each(seq, view.begin(), view.end(), [&](auto entity) {
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
						ctx.cptr()
					);
				});
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
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
					action
				);
			} else {
				_execute_system_trivial_default<SystemT, ChildSystemsListT>(
					info,
					parent,
					action
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
				auto& after = ctx.info.registry.get<C>(ctx.entity);

				if(before.value != after) {
					ctx.info.registry.emplace<component_changed<C>>(ctx.entity);
				}
				ctx.info.registry.remove<beforechange_storage<C>>(ctx.entity);
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_user
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			)
		{
			using boost::mp11::mp_for_each;
			using std::execution::seq;

			static_assert(!SystemT::has_trivial_impl);

			auto view = ecsact_entt_view(
				std::type_identity<SystemT>{},
				info.registry
			);

			std::for_each(seq, view.begin(), view.end(), [&](auto entity) {
				using boost::mp11::mp_empty;
				using boost::mp11::mp_size;

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
						ctx.cptr()
					);
				});
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			)
		{
			if constexpr(is_action<SystemT>()) {
				auto& actions = std::get<std::vector<SystemT>>(info.actions);
				for(const auto& action : std::get<std::vector<SystemT>>(info.actions)) {
					if constexpr(SystemT::has_trivial_impl) {
						_execute_system_trivial<SystemT, ChildSystemsListT>(
							info,
							parent,
							&action
						);
					} else {
						_execute_system_user<SystemT, ChildSystemsListT>(
							info,
							parent,
							&action
						);
					}
				}
				actions.clear();
			} else {
				if constexpr(SystemT::has_trivial_impl) {
					_execute_system_trivial<SystemT, ChildSystemsListT>(
						info,
						parent,
						nullptr
					);
				} else {
					_execute_system_user<SystemT, ChildSystemsListT>(
						info,
						parent,
						nullptr
					);
				}
			}
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

		void _trigger_add_component_events
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				using namespace ::entt::literals;

				::entt::basic_view added_view{
					info.registry.storage<C>(),
					info.registry.storage<component_added<C>>(),
				};
				
				for(entt_entity_type entity : added_view) {
					if constexpr(std::is_empty_v<C>) {
						_invoke_add_callbacks<C>(info, info.ecsact_entity_id(entity), C{});
					} else {
						_invoke_add_callbacks<C>(
							info,
							info.ecsact_entity_id(entity),
							added_view.get<C>(entity)
						);
					}
				}
				
				info.registry.clear<component_added<C>>();
			});
		}

		void _trigger_update_component_events
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				using namespace ::entt::literals;

				::entt::basic_view changed_view{
					info.registry.storage<C>(),
					info.registry.storage<component_changed<C>>(),
				};
				
				for(entt_entity_type entity : changed_view) {
					if constexpr(std::is_empty_v<C>) {
						_invoke_update_callbacks<C>(
							info,
							info.ecsact_entity_id(entity),
							C{}
						);
					} else {
						_invoke_update_callbacks<C>(
							info,
							info.ecsact_entity_id(entity),
							changed_view.get<C>(entity)
						);
					}
				}
				
				info.registry.clear<component_changed<C>>();
			});
		}

		void _trigger_remove_component_events
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				using namespace ::entt::literals;

				::entt::basic_view before_removed_view{
					info.registry.storage<detail::temp_storage<C>>(),
					info.registry.storage<component_removed<C>>(),
				};

				for(entt_entity_type entity : before_removed_view) {
					_invoke_before_remove_callbacks<C>(info, entity, before_removed_view);
					info.registry.storage<detail::temp_storage<C>>().remove(entity);
				}

				::entt::basic_view after_removed_view{
					info.registry.storage<component_removed<C>>(),
				};

				for(entt_entity_type entity : after_removed_view) {
					_invoke_after_remove_callbacks<C>(
						info,
						info.ecsact_entity_id(entity)
					);
				}

				info.registry.clear<component_removed<C>>();
			});
		}

		void _execute_systems
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::execution_order>(
				[&]<typename SystemList>(SystemList) {
					mp_for_each<SystemList>([&]<typename SystemPair>(SystemPair) {
						using boost::mp11::mp_first;
						using boost::mp11::mp_second;

						using SystemT = mp_first<SystemPair>;
						using ChildSystemsListT = mp_second<SystemPair>;
						_execute_system<SystemT, ChildSystemsListT>(
							info,
							nullptr
						);
					});
				}
			);
		}

	public:
		void execute_systems
			( ::ecsact::registry_id reg_id
			)
		{
			using boost::mp11::mp_for_each;

			std::mutex mutex;
			auto& info = _registries.at(reg_id);
			info.mutex = std::ref(mutex);

			_sort_components(info);
			_execute_systems(info);
			_clear_transients(info);
			_trigger_add_component_events(info);
			_trigger_update_component_events(info);
			_trigger_remove_component_events(info);

			info.mutex = std::nullopt;
		}

	};

}
