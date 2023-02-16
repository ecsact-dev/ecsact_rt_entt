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
#include <algorithm>
#include <boost/mp11.hpp>
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/definitions.h"
#include "ecsact/runtime/core.h"
#include "ecsact/lib.hh"
#include <entt/entt.hpp>
#include "ecsact/entt/detail/mp11_util.hh"
#include "ecsact/entt/detail/system_execution_context.hh"
#include "ecsact/entt/detail/execution_events_collector.hh"
#include "ecsact/entt/detail/registry_info.hh"
#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/system_view.hh"
#include "ecsact/entt/trivial_system_impl.hh"

namespace ecsact::entt {
template<typename Package>
class runtime {
	/**
	 * Checks if type T is listd as one of the actions in the ecact package.
	 * @returns `true` if T is a component belonging to `package`, `false`
	 *          otherwise.
	 */
	template<typename T>
	static constexpr bool is_action() {
		using boost::mp11::mp_any;
		using boost::mp11::mp_apply;
		using boost::mp11::mp_bind_front;
		using boost::mp11::mp_transform_q;

		return mp_apply<
			mp_any,
			mp_transform_q<
				mp_bind_front<std::is_same, std::remove_cvref_t<T>>,
				typename Package::actions>>::value;
	}

	using registry_info = ecsact_entt_rt::registry_info<Package>;

	using registries_map_t =
		std::unordered_map<ecsact_registry_id, registry_info>;

	using actions_span_t = std::span<ecsact_action, std::dynamic_extent>;

	ecsact_registry_id _last_registry_id{};
	registries_map_t   _registries;

#ifdef ECSACT_ENTT_RUNTIME_DYNAMIC_SYSTEM_IMPLS
	using sys_impl_fns_t =
		std::unordered_map<ecsact_system_like_id, ecsact_system_execution_impl>;
	sys_impl_fns_t _sys_impl_fns;
#endif

public:
	template<typename SystemT>
	using system_execution_context = ecsact_entt_rt::system_execution_context<
		Package,
		ecsact::system_capabilities_info<SystemT>>;
	using execution_events_collector = ecsact_entt_rt::execution_events_collector;
	using registry_type = ::entt::registry;
	using entt_entity_type = typename registry_type::entity_type;
	using package = Package;

	ecsact_registry_id create_registry(const char* registry_name) {
		using boost::mp11::mp_for_each;

		// Using the index of _registries as an ID
		const auto reg_id =
			static_cast<ecsact_registry_id>(static_cast<int>(_last_registry_id) + 1);

		_last_registry_id = reg_id;

		auto itr = _registries.emplace_hint(
			_registries.end(),
			std::piecewise_construct,
			std::forward_as_tuple(reg_id),
			std::forward_as_tuple()
		);

		registry_info& info = itr->second;
		info.init_registry();
		return reg_id;
	}

	void destroy_registry(ecsact_registry_id reg_id) {
		_registries.erase(reg_id);
	}

	void clear_registry(ecsact_registry_id reg_id) {
		using boost::mp11::mp_for_each;

		auto& info = _registries.at(reg_id);

		info.registry = {};
		info.init_registry();
		info.entities_map.clear();
		info._ecsact_entity_ids.clear();
		info.last_entity_id = {};
	}

	ecsact_entity_id create_entity(ecsact_registry_id reg_id) {
		std::mutex mutex;
		auto&      info = _registries.at(reg_id);
		info.mutex = mutex;
		auto new_entity_id = info.create_entity().ecsact_entity_id;
		info.mutex = std::nullopt;
		return new_entity_id;
	}

	void ensure_entity(ecsact_registry_id reg_id, ecsact_entity_id entity_id) {
		auto& info = _registries.at(reg_id);
		if(!info.entities_map.contains(entity_id)) {
			std::mutex mutex;
			info.mutex = mutex;
			info.create_entity(entity_id);
			info.mutex = std::nullopt;
		}
	}

	bool entity_exists(ecsact_registry_id reg_id, ecsact_entity_id entity_id) {
		auto& info = _registries.at(reg_id);
		return info.entities_map.contains(entity_id);
	}

	void destroy_entity(ecsact_registry_id reg_id, ecsact_entity_id entity_id) {
		using boost::mp11::mp_for_each;

		auto& info = _registries.at(reg_id);

		info.destroy_entity(entity_id);
	}

	int count_entities(ecsact_registry_id reg_id) {
		auto& info = _registries.at(reg_id);
		return static_cast<int>(info.registry.alive());
	}

	std::vector<ecsact_entity_id> get_entities(ecsact_registry_id reg_id) {
		auto&                         info = _registries.at(reg_id);
		std::vector<ecsact_entity_id> result;
		for(auto& entry : info.entities_map) {
			result.push_back(entry.first);
		}

		return result;
	}

	void get_entities(
		ecsact_registry_id reg_id,
		int                max_entities_count,
		ecsact_entity_id*  out_entities,
		int*               out_entities_count
	) {
		auto& info = _registries.at(reg_id);

		int entities_count = static_cast<int>(info.entities_map.size());
		max_entities_count = std::min(entities_count, max_entities_count);

		auto itr = info.entities_map.begin();
		for(int i = 0; max_entities_count > i; ++i) {
			if(itr == info.entities_map.end()) {
				break;
			}

			out_entities[i] = itr->first;
			++itr;
		}

		if(out_entities_count != nullptr) {
			*out_entities_count = entities_count;
		}
	}

	template<typename C>
	ecsact_add_error add_component(
		ecsact_registry_id reg_id,
		ecsact_entity_id   entity_id,
		const C&           component_data
	) {
		auto& info = _registries.at(reg_id);
		auto  entt_entity_id = info.entities_map.at(entity_id);

		constexpr auto fields_info = ecsact::fields_info<C>();
		if constexpr(!fields_info.empty()) {
			for(auto& field : fields_info) {
				if(field.storage_type == ECSACT_ENTITY_TYPE) {
					auto entity_field =
						field.template get<ecsact_entity_id>(&component_data);
					if(!info.entities_map.contains(entity_field)) {
						return ECSACT_ADD_ERR_ENTITY_INVALID;
					}
				}
			}
		}

		if constexpr(std::is_empty_v<C>) {
			info.template add_component<C>(entt_entity_id);
		} else {
			info.template add_component<C>(entt_entity_id, component_data);
		}

		return ECSACT_ADD_OK;
	}

	template<typename ComponentT>
	ecsact_add_error add_component(
		ecsact_registry_id reg_id,
		ecsact_entity_id   entity_id
	) {
		return add_component<ComponentT>(reg_id, entity_id, ComponentT{});
	}

	ecsact_add_error add_component(
		ecsact_registry_id  reg_id,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data
	) {
		using boost::mp11::mp_for_each;

		ecsact_add_error err = ECSACT_ADD_OK;

		mp_for_each<typename package::components>([&]<typename C>(const C&) {
			if(C::id == component_id) {
				if constexpr(std::is_empty_v<C>) {
					err = add_component<C>(reg_id, entity_id);
				} else {
					err = add_component<C>(
						reg_id,
						entity_id,
						*static_cast<const C*>(component_data)
					);
				}
			}
		});

		return err;
	}

	template<typename ComponentT>
	bool has_component(ecsact_registry_id reg_id, ecsact_entity_id entity_id) {
		auto& info = _registries.at(reg_id);
		auto  entt_entity_id = info.entities_map.at(entity_id);

		return info.registry.template all_of<ComponentT>(entt_entity_id);
	}

	bool has_component(
		ecsact_registry_id  reg_id,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id
	) {
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
	const ComponentT& get_component(
		ecsact_registry_id reg_id,
		ecsact_entity_id   entity_id
	) {
		auto& info = _registries.at(reg_id);
		auto  entt_entity_id = info.entities_map.at(entity_id);

		return info.registry.template get<ComponentT>(entt_entity_id);
	}

	const void* get_component(
		ecsact_registry_id  reg_id,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id
	) {
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

	int count_components(
		ecsact_registry_id registry_id,
		ecsact_entity_id   entity_id
	) {
		using boost::mp11::mp_for_each;

		int count = 0;
		mp_for_each<typename package::components>([&]<typename C>(C) {
			if(has_component<C>(registry_id, entity_id)) {
				count += 1;
			}
		});
		return count;
	}

	void each_component(
		ecsact_registry_id             registry_id,
		ecsact_entity_id               entity_id,
		ecsact_each_component_callback callback,
		void*                          callback_user_data
	) {
		using boost::mp11::mp_for_each;

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if(has_component<C>(registry_id, entity_id)) {
				if constexpr(std::is_empty_v<C>) {
					callback(
						static_cast<ecsact_component_id>(C::id),
						nullptr,
						callback_user_data
					);
				} else {
					callback(
						static_cast<ecsact_component_id>(C::id),
						&get_component<C>(registry_id, entity_id),
						callback_user_data
					);
				}
			}
		});
	}

	void get_components(
		ecsact_registry_id   registry_id,
		ecsact_entity_id     entity_id,
		int                  max_components_count,
		ecsact_component_id* out_component_ids,
		const void**         out_components_data,
		int*                 out_components_count
	) {
		using boost::mp11::mp_for_each;

		int index = 0;
		mp_for_each<typename package::components>([&]<typename C>(C) {
			if(index >= max_components_count) {
				return;
			}

			if(has_component<C>(registry_id, entity_id)) {
				out_component_ids[index] = C::id;
				if constexpr(std::is_empty_v<C>) {
					out_components_data[index] = nullptr;
				} else {
					out_components_data[index] =
						&get_component<C>(registry_id, entity_id);
				}
				index += 1;
			}
		});

		if(out_components_count != nullptr) {
			*out_components_count = index;
		}
	}

	template<typename ComponentT>
	ecsact_update_error update_component(
		ecsact_registry_id reg_id,
		ecsact_entity_id   entity_id,
		const ComponentT&  component_data
	) {
		auto& info = _registries.at(reg_id);
		auto  entt_entity_id = info.entities_map.at(entity_id);

		constexpr auto fields_info = ecsact::fields_info<ComponentT>();
		if constexpr(!fields_info.empty()) {
			for(auto& field : fields_info) {
				if(field.storage_type == ECSACT_ENTITY_TYPE) {
					auto entity_field =
						field.template get<ecsact_entity_id>(&component_data);
					if(!info.entities_map.contains(entity_field)) {
						return ECSACT_UPDATE_ERR_ENTITY_INVALID;
					}
				}
			}
		}

		auto& component = info.registry.template get<ComponentT>(entt_entity_id);
		component = component_data;

		return ECSACT_UPDATE_OK;
	}

	ecsact_update_error update_component(
		ecsact_registry_id  reg_id,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id,
		const void*         component_data
	) {
		using boost::mp11::mp_for_each;

		std::optional<ecsact_update_error> result;
		mp_for_each<typename package::components>([&]<typename C>(const C&) {
			if(C::id == component_id) {
				if constexpr(!std::is_empty_v<C>) {
					result = update_component<C>(
						reg_id,
						entity_id,
						*static_cast<const C*>(component_data)
					);
				}
			}
		});
		return *result;
	}

	template<typename C>
	void remove_component(ecsact_registry_id reg_id, ecsact_entity_id entity_id) {
		auto& info = _registries.at(reg_id);
		auto  entt_entity_id = info.entities_map.at(entity_id);

		info.template remove_component<C>(entt_entity_id);
	}

	void remove_component(
		ecsact_registry_id  reg_id,
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id
	) {
		using boost::mp11::mp_for_each;

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if(C::id == component_id) {
				remove_component<C>(reg_id, entity_id);
			}
		});
	}

	size_t component_size(ecsact_component_id comp_id) {
		using boost::mp11::mp_for_each;

		size_t comp_size = 0;
		mp_for_each<typename package::components>([&]<typename C>(C) {
			if(C::id == comp_id) {
				comp_size = sizeof(C);
			}
		});
		return comp_size;
	}

	size_t action_size(ecsact_action_id action_id) {
		using boost::mp11::mp_for_each;

		size_t act_size = 0;
		mp_for_each<typename package::actions>([&]<typename A>(A) {
			if(A::id == action_id) {
				act_size = sizeof(A);
			}
		});
		return act_size;
	}

private:
	template<typename SystemT>
	void _apply_pending_adds(registry_info& info) {
		using boost::mp11::mp_first;
		using boost::mp11::mp_flatten;
		using boost::mp11::mp_for_each;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_map_find;
		using boost::mp11::mp_push_back;
		using boost::mp11::mp_size;
		using boost::mp11::mp_transform;
		using boost::mp11::mp_unique;
		using ecsact::entt::detail::pending_add;
		using ecsact::entt_mp11_util::mp_map_find_value_or;

		using caps_info = ecsact::system_capabilities_info<SystemT>;
		using associations = typename caps_info::associations;

		using system_generates = mp_transform<
			mp_first,
			mp_flatten<typename caps_info::generates, ::ecsact::mp_list<>>>;
		using adds_components = typename caps_info::adds_components;
		static_assert(!std::is_same_v<system_generates, void>);

		using addables = mp_unique<mp_flatten<
			mp_flatten<
				mp_push_back<system_generates, adds_components>,
				::ecsact::mp_list<>>,
			::ecsact::mp_list<>>>;

		auto for_each_addable = [&]<typename C>(C) {
			using boost::mp11::mp_apply;
			using boost::mp11::mp_bind_front;
			using boost::mp11::mp_transform_q;
			using boost::mp11::mp_any;

			// Making sure all the components in `addables` list are indeed package
			// components. If this assertion fails there was an error in creating
			// the `addables` alias.
			static_assert(mp_apply<
										mp_any,
										mp_transform_q<
											mp_bind_front<std::is_same, std::remove_cvref_t<C>>,
											typename Package::components>>::value);

			auto view = info.registry.template view<pending_add<C>>();
			if constexpr(std::is_empty_v<C>) {
				view.each([&](auto entity) { info.template add_component<C>(entity); });
			} else {
				view.each([&](auto entity, auto& component) {
					info.template add_component<C>(entity, component.value);
				});
			}

			info.registry.template clear<pending_add<C>>();
		};

		mp_for_each<addables>(for_each_addable);

		mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
			using boost::mp11::mp_at;
			using boost::mp11::mp_size_t;

			using Assoc = mp_at<associations, mp_size_t<I>>;
			using addables = typename Assoc::adds_components;

			mp_for_each<addables>(for_each_addable);
		});
	}

	template<typename SystemT>
	void _apply_pending_removes(registry_info& info) {
		using boost::mp11::mp_for_each;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_size;
		using ecsact::entt::detail::pending_remove;
		using ecsact::entt_mp11_util::mp_map_find_value_or;

		using caps_info = ecsact::system_capabilities_info<SystemT>;
		using associations = typename caps_info::associations;

		using removes_components = typename caps_info::removes_components;

		auto for_each_removable = [&]<typename C>(C) {
			auto view = info.registry.template view<pending_remove<C>>();
			view.each([&](auto entity) { info.template remove_component<C>(entity); }
			);

			info.registry.template clear<pending_remove<C>>();
		};

		mp_for_each<removes_components>(for_each_removable);

		mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
			using boost::mp11::mp_at;
			using boost::mp11::mp_size_t;

			using Assoc = mp_at<associations, mp_size_t<I>>;
			using removes_components = typename Assoc::removes_components;

			mp_for_each<removes_components>(for_each_removable);
		});
	}

	template<typename ChildSystemsListT>
	void _execute_systems_list(
		registry_info&                   info,
		ecsact_system_execution_context* parent,
		const actions_span_t&            actions
	) {
		using boost::mp11::mp_for_each;

		mp_for_each<ChildSystemsListT>([&]<typename SystemPair>(SystemPair) {
			using boost::mp11::mp_first;
			using boost::mp11::mp_second;
			using ChildSystemT = mp_first<SystemPair>;
			using GrandChildSystemsListT = mp_second<SystemPair>;

			_execute_system<ChildSystemT, GrandChildSystemsListT>(
				info,
				parent,
				actions
			);
		});
	}

	template<typename SystemT, typename ChildSystemsListT>
	void _execute_system_trivial(
		registry_info&                   info,
		ecsact_system_execution_context* parent,
		const actions_span_t&            actions
	) {
		using boost::mp11::mp_empty;
		const auto system_id = ecsact_id_cast<ecsact_system_like_id>(SystemT::id);

		const void* action_data = nullptr;

		auto each_cb = [&](auto& view, auto& assoc_views, auto entity) {
			if constexpr(!mp_empty<ChildSystemsListT>::value) {
				system_execution_context<SystemT>
					ctx(info, system_id, view, assoc_views, entity, parent, action_data);
				_execute_systems_list<ChildSystemsListT>(info, ctx.cptr(), actions);
			}
		};

		if constexpr(is_action<SystemT>()) {
			for(auto& action : actions) {
				if(action.action_id == SystemT::id) {
					action_data = action.action_data;
					trivial_system_impl<SystemT>(info, each_cb);
				}
			}
		} else {
			trivial_system_impl<SystemT>(info, each_cb);
		}
	}

	template<typename SystemT, typename ChildSystemsListT>
	void _execute_system_user_itr(
		registry_info&                          info,
		system_view_type<SystemT>&              view,
		system_association_views_type<SystemT>& assoc_views,
		entt_entity_type                        entity,
		ecsact_system_execution_context*        parent,
		const void*                             action,
		const actions_span_t&                   actions
	) {
		[[maybe_unused]] const auto system_name = typeid(SystemT).name();
		const auto system_id = ecsact_id_cast<ecsact_system_like_id>(SystemT::id);

		system_execution_context<SystemT>
			ctx(info, system_id, view, assoc_views, entity, parent, action);

		// Execute the user defined system implementation
#ifdef ECSACT_ENTT_RUNTIME_DYNAMIC_SYSTEM_IMPLS
		if(_sys_impl_fns.contains(system_id)) {
			_sys_impl_fns.at(system_id)(ctx.cptr());
		}
#	ifdef ECSACT_ENTT_RUNTIME_STATIC_SYSTEM_IMPLS
		else
#	endif
#endif

#ifdef ECSACT_ENTT_RUNTIME_STATIC_SYSTEM_IMPLS
		{
			typename SystemT::context sys_cpp_ctx{ctx.cptr()};
			SystemT::impl(sys_cpp_ctx);
		}
#endif

		_execute_systems_list<ChildSystemsListT>(info, ctx.cptr(), actions);
	}

	template<typename SystemT, typename ChildSystemsListT>
	void _execute_system_user(
		registry_info&                   info,
		ecsact_system_execution_context* parent,
		const actions_span_t&            actions
	) {
		using boost::mp11::mp_for_each;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_size;
		using boost::mp11::mp_size_t;

		using caps_info = ecsact::system_capabilities_info<SystemT>;
		using associations = typename caps_info::associations;

		auto assoc_views = system_association_views<SystemT>(info.registry);
		auto assoc_views_itrs = system_association_views_iterators(assoc_views);

		auto        view = system_view<SystemT>(info.registry);
		const void* action_data = nullptr;

		auto itr_view = [&] {
			for(auto entity : view) {
				bool missing_assoc_entities = false;
				mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
					using boost::mp11::mp_at;
					using boost::mp11::mp_size_t;

					using Assoc = mp_at<associations, mp_size_t<I>>;
					using ComponentT = typename Assoc::component_type;

					constexpr auto offset = Assoc::field_offset;

					auto& assoc_view = std::get<I>(assoc_views);
					auto& assoc_view_itr = std::get<I>(assoc_views_itrs);
					if(assoc_view.begin() == assoc_view.end()) {
						missing_assoc_entities = true;
						return;
					}

					assert(view.contains(entity));
					auto& comp = view.template get<ComponentT>(entity);

					auto field_entity_value = *reinterpret_cast<const ecsact_entity_id*>(
						reinterpret_cast<const char*>(&comp) + offset
					);
					auto entt_field_entity_value =
						info.get_entt_entity_id(field_entity_value);

					bool found_associated_entity = *assoc_view_itr ==
						entt_field_entity_value;
					if(!found_associated_entity) {
						assoc_view_itr = assoc_view.begin();
						for(; assoc_view_itr != assoc_view.end(); ++assoc_view_itr) {
							found_associated_entity = *assoc_view_itr ==
								entt_field_entity_value;
							if(found_associated_entity) {
								break;
							}
						}
					}

					if(!found_associated_entity) {
						missing_assoc_entities = true;
					}
				});

				if(!missing_assoc_entities) {
					_execute_system_user_itr<SystemT, ChildSystemsListT>(
						info,
						view,
						assoc_views,
						entity,
						parent,
						action_data,
						actions
					);

					mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
						++std::get<I>(assoc_views_itrs);
					});
				}
			}
		};

		if constexpr(is_action<SystemT>()) {
			for(auto& action : actions) {
				if(action.action_id == SystemT::id) {
					action_data = action.action_data;
					itr_view();
				}
			}
		} else {
			itr_view();
		}
	}

	template<typename SystemT, typename ChildSystemsListT>
	void _execute_system(
		registry_info&                   info,
		ecsact_system_execution_context* parent,
		const actions_span_t&            actions
	) {
		if constexpr(is_trivial_system<SystemT>()) {
			_execute_system_trivial<SystemT, ChildSystemsListT>(
				info,
				parent,
				actions
			);
		} else {
			_execute_system_user<SystemT, ChildSystemsListT>(info, parent, actions);
		}

		_apply_pending_removes<SystemT>(info);
		_apply_pending_adds<SystemT>(info);
	}

	void _clear_transients(registry_info& info) {
		using boost::mp11::mp_for_each;

		mp_for_each<typename package::transients>([&]<typename C>(C) {
			info.registry.template clear<C>();
		});
	}

	void _sort_components(registry_info& info) {
		using boost::mp11::mp_for_each;

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if constexpr(!std::is_empty_v<C> && !C::transient) {
				// Sorting for deterministic order of components when executing
				// systems.
				// TODO(zaucy): This sort is only necessary for components part of a
				//              system execution hierarchy greater than 1.
				info.registry.template sort<C>([](const C& a, const C& b) -> bool {
					return a < b;
				});
			}
		});
	}

	void _trigger_init_component_events(
		registry_info&              info,
		execution_events_collector& events_collector
	) {
		using boost::mp11::mp_for_each;

		if(!events_collector.has_init_callback()) {
			return;
		}

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if constexpr(C::transient) {
				return;
			}

			::entt::basic_view added_view{
				info.registry.template storage<C>(),
				info.registry.template storage<component_added<C>>(),
			};

			for(entt_entity_type entity : added_view) {
				if constexpr(std::is_empty_v<C>) {
					events_collector.invoke_init_callback<C>(
						info.get_ecsact_entity_id(entity)
					);
				} else {
					events_collector.invoke_init_callback<C>(
						info.get_ecsact_entity_id(entity),
						added_view.template get<C>(entity)
					);
				}
			}
		});
	}

	void _trigger_update_component_events(
		registry_info&              info,
		execution_events_collector& events_collector
	) {
		using boost::mp11::mp_for_each;
		using detail::beforechange_storage;

		if(!events_collector.has_update_callback()) {
			return;
		}

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if constexpr(!C::transient && !std::is_empty_v<C>) {
				::entt::basic_view changed_view{
					info.registry.template storage<C>(),
					info.registry.template storage<beforechange_storage<C>>(),
					info.registry.template storage<component_changed<C>>(),
				};

				for(entt_entity_type entity : changed_view) {
					auto& before =
						changed_view.template get<beforechange_storage<C>>(entity);
					auto& current = changed_view.template get<C>(entity);

					if(before.value != current) {
						events_collector.invoke_update_callback<C>(
							info.get_ecsact_entity_id(entity),
							current
						);
					}
					before.set = false;
				}
			}
		});
	}

	void _trigger_remove_component_events(
		registry_info&              info,
		execution_events_collector& events_collector
	) {
		using boost::mp11::mp_for_each;

		if(!events_collector.has_remove_callback()) {
			return;
		}

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if constexpr(C::transient) {
				return;
			}

			if constexpr(std::is_empty_v<C>) {
				::entt::basic_view removed_view{
					info.registry.template storage<component_removed<C>>(),
				};
				for(entt_entity_type entity : removed_view) {
					events_collector.invoke_remove_callback<C>(
						info.get_ecsact_entity_id(entity)
					);
				}
			} else {
				::entt::basic_view removed_view{
					info.registry.template storage<detail::temp_storage<C>>(),
					info.registry.template storage<component_removed<C>>(),
				};
				for(entt_entity_type entity : removed_view) {
					events_collector.invoke_remove_callback<C>(
						info.get_ecsact_entity_id(entity),
						removed_view.template get<detail::temp_storage<C>>(entity).value
					);
					info.registry.template storage<detail::temp_storage<C>>().remove(
						entity
					);
				}
			}
		});
	}

	void _trigger_create_entity_event(
		registry_info&              info,
		execution_events_collector& events_collector
	) {
		using boost::mp11::mp_for_each;
		using ecsact::entt::detail::created_entity;

		if(!events_collector.has_entity_created_callback()) {
			return;
		}

		::entt::basic_view created_view{
			info.registry.template storage<created_entity>(),
		};

		for(entt_entity_type entity : created_view) {
			events_collector.invoke_entity_created_callback(
				info.get_ecsact_entity_id(entity),
				created_view.get<created_entity>(entity).index
			);
		}
	}

	void _trigger_destroy_entity_event(
		registry_info&              info,
		execution_events_collector& events_collector
	) {
		using boost::mp11::mp_for_each;
		using ecsact::entt::detail::destroyed_entity;

		if(!events_collector.has_entity_destroyed_callback()) {
			return;
		}

		::entt::basic_view destroy_view{
			info.registry.template storage<destroyed_entity>(),
		};

		for(entt_entity_type entity : destroy_view) {
			events_collector.invoke_entity_destroyed_callback(
				info.get_ecsact_entity_id(entity),
				destroy_view.get<destroyed_entity>(entity).index
			);
		}
	}

	void _execute_systems(registry_info& info, actions_span_t& actions) {
		using boost::mp11::mp_for_each;

		mp_for_each<typename package::execution_order>(
			[&]<typename SystemPair>(SystemPair) {
				using boost::mp11::mp_first;
				using boost::mp11::mp_second;

				using SystemT = mp_first<SystemPair>;
				using ChildSystemsListT = mp_second<SystemPair>;
				_execute_system<SystemT, ChildSystemsListT>(info, nullptr, actions);
			}
		);
	}

	template<typename C>
		requires(!std::is_empty_v<C>)
	void _pre_exec_add_component(
		registry_info&   info,
		entt_entity_type entity,
		const C&         component
	) {
#ifndef NDEBUG
		{
			const bool already_has_component =
				info.registry.template all_of<C>(entity);
			if(already_has_component) {
				using namespace std::string_literals;
				std::string err_msg = "Entity already has component. ";
				err_msg += "Attempted added component: "s + typeid(C).name();
				throw std::runtime_error(err_msg.c_str());
			}
		}
#endif

		info.template add_component<C>(entity, component);
		info.registry.template emplace<component_added<C>>(entity);
	}

	template<typename C>
		requires(std::is_empty_v<C>)
	void _pre_exec_add_component(registry_info& info, entt_entity_type entity) {
#ifndef NDEBUG
		if(info.registry.template all_of<C>(entity)) {
			using namespace std::string_literals;
			std::string err_msg = "Entity already has component. ";
			err_msg += "Attempted added component: "s + typeid(C).name();
			throw std::runtime_error(err_msg.c_str());
		}
#endif

		info.template add_component<C>(entity);
		info.registry.template emplace<component_added<C>>(entity);
	}

	template<typename C>
		requires(!std::is_empty_v<C>)
	void _pre_exec_update_component(
		registry_info&   info,
		entt_entity_type entity,
		const C&         updated_component
	) {
		using detail::beforechange_storage;

#ifndef NDEBUG
		if(!info.registry.template all_of<C>(entity)) {
			using namespace std::string_literals;
			std::string err_msg = "Entity does not have component. ";
			err_msg += "Attempted update on component: "s + typeid(C).name();
			throw std::runtime_error(err_msg.c_str());
		}
#endif

		C& component = info.registry.template get<C>(entity);
		if(info.registry.template all_of<beforechange_storage<C>>(entity)) {
			auto& before =
				info.registry.template get<beforechange_storage<C>>(entity);
			if(!before.set) {
				before.value = component;
				before.set = true;
			}
		} else {
			info.registry
				.template emplace<beforechange_storage<C>>(entity, component, true);
		}

		component = updated_component;

		if(!info.registry.template all_of<component_added<C>>(entity)) {
			info.registry.template emplace_or_replace<component_changed<C>>(entity);
		}
	}

	template<typename C>
	void _pre_exec_remove_component(
		registry_info&   info,
		entt_entity_type entity
	) {
		using detail::temp_storage;

		if(info.registry.template all_of<component_added<C>>(entity)) {
			info.registry.template remove<component_added<C>>(entity);
		}
		if constexpr(!std::is_empty_v<C>) {
			auto& temp = info.registry.template storage<temp_storage<C>>();

			// Store current value of component for the before_remove event later
			if(temp.contains(entity)) {
				temp.get(entity).value = info.registry.template get<C>(entity);
			} else {
				temp.emplace(entity, info.registry.template get<C>(entity));
			}
		}

		info.template remove_component<C>(entity);
		if(!info.registry.template all_of<component_removed<C>>(entity)) {
			info.registry.template emplace<component_removed<C>>(entity);
		}
		info.registry.template remove<component_changed<C>>(entity);
		info.registry.template remove<component_added<C>>(entity);
	}

	void _apply_execution_options(
		const ecsact_execution_options& options,
		registry_info&                  info
	) {
		using boost::mp11::mp_for_each;
		using ecsact::entt::detail::created_entity;
		using ecsact::entt::detail::destroyed_entity;

		for(int i = 0; options.create_entities_length > i; i++) {
			auto entity = info.create_entity().entt_entity_id;
			info.registry.template emplace<created_entity>(entity, i);

			for(int j = 0; options.create_entities_components_length[i] > j; j++) {
				const ecsact_component& comp = options.create_entities_components[i][j];

				mp_for_each<typename package::components>([&]<typename C>(C) {
					if constexpr(C::transient) {
						return;
					}

					if(comp.component_id == static_cast<ecsact_component_id>(C::id)) {
						if constexpr(std::is_empty_v<C>) {
							_pre_exec_add_component<C>(info, entity);
						} else {
							_pre_exec_add_component<C>(
								info,
								entity,
								*static_cast<const C*>(comp.component_data)
							);
						}
					}
				});
			}
		}

		for(int i = 0; options.add_components_length > i; ++i) {
			const ecsact_entity_id& entity = options.add_components_entities[i];
			const ecsact_component& comp = options.add_components[i];

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) {
					return;
				}

				if(comp.component_id == static_cast<ecsact_component_id>(C::id)) {
					if constexpr(std::is_empty_v<C>) {
						_pre_exec_add_component<C>(
							info,
							info.entities_map.at(static_cast<ecsact_entity_id>(entity))
						);
					} else {
						_pre_exec_add_component<C>(
							info,
							info.entities_map.at(static_cast<ecsact_entity_id>(entity)),
							*static_cast<const C*>(comp.component_data)
						);
					}
				}
			});
		}

		for(int i = 0; options.update_components_length > i; ++i) {
			const ecsact_entity_id& entity = options.update_components_entities[i];
			const ecsact_component& comp = options.update_components[i];

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) {
					return;
				}

				if(comp.component_id == static_cast<ecsact_component_id>(C::id)) {
					if constexpr(!std::is_empty_v<C>) {
						_pre_exec_update_component<C>(
							info,
							info.entities_map.at(entity),
							*static_cast<const C*>(comp.component_data)
						);
					} else {
						assert(!std::is_empty_v<C>);
					}
				}
			});
		}

		for(int i = 0; options.remove_components_length > i; ++i) {
			const ecsact_entity_id& entity = options.remove_components_entities[i];
			ecsact_component_id     component_id = options.remove_components[i];

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) {
					return;
				}

				if(component_id == static_cast<ecsact_component_id>(C::id)) {
					_pre_exec_remove_component<C>(
						info,
						info.entities_map.at(static_cast<ecsact_entity_id>(entity))
					);
				}
			});
		}

		for(int i = 0; options.destroy_entities_length > i; ++i) {
			const ecsact_entity_id& entity = options.destroy_entities[i];
			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(C::transient) {
					return;
				}
				if(info.registry.template all_of<C>(info.get_entt_entity_id(entity))) {
					_pre_exec_remove_component<C>(
						info,
						info.entities_map.at(static_cast<ecsact_entity_id>(entity))
					);
				}
			});
			auto entt_id = info.get_entt_entity_id(entity);
			info.registry.template emplace<destroyed_entity>(entt_id, i);
			info.destroy_entity(entity);
		}
	}

	void _clear_event_markers(registry_info& info) {
		using boost::mp11::mp_for_each;
		using ecsact::entt::detail::created_entity;
		using ecsact::entt::detail::destroyed_entity;

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if constexpr(C::transient) {
				return;
			}

			info.registry.template clear<component_added<C>>();
		});

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if constexpr(C::transient) {
				return;
			}

			info.registry.template storage<component_changed<C>>().clear();
		});

		mp_for_each<typename package::components>([&]<typename C>(C) {
			if constexpr(C::transient) {
				return;
			}

			info.registry.template clear<component_removed<C>>();
		});

		info.registry.template clear<created_entity>();
		info.registry.template clear<destroyed_entity>();
	}

	auto _validate_action(ecsact_registry_id registry_id, ecsact_action& action)
		-> ecsact_execute_systems_error {
		using boost::mp11::mp_for_each;

		auto& info = _registries.at(registry_id);
		auto  result = ECSACT_EXEC_SYS_OK;

		mp_for_each<typename package::actions>([&]<typename A>(A) {
			if(A::id != action.action_id) {
				return;
			}
			constexpr auto fields_info = ecsact::fields_info<A>();
			for(auto& field : fields_info) {
				if(field.storage_type == ECSACT_ENTITY_TYPE) {
					auto entity_field =
						field.template get<ecsact_entity_id>(action.action_data);
					if(!info.entities_map.contains(entity_field)) {
						result = ECSACT_EXEC_SYS_ERR_ACTION_ENTITY_INVALID;
					}
				}
			}
		});

		return result;
	}

public:
#ifdef ECSACT_ENTT_RUNTIME_DYNAMIC_SYSTEM_IMPLS
	bool set_system_execution_impl(
		ecsact_system_like_id        system_id,
		ecsact_system_execution_impl exec_impl
	) {
		if(exec_impl == nullptr) {
			_sys_impl_fns.erase(system_id);
		} else {
			_sys_impl_fns[system_id] = exec_impl;
		}
		return true;
	}
#endif

	ecsact_execute_systems_error execute_systems(
		ecsact_registry_id                        reg_id,
		int                                       execution_count,
		const ecsact_execution_options*           execution_options_list,
		std::optional<execution_events_collector> events_collector
	) {
		auto  mutex = std::mutex{};
		auto& info = _registries.at(reg_id);
		auto  exec_err = ECSACT_EXEC_SYS_OK;
		info.mutex = std::ref(mutex);

		if(execution_options_list != nullptr) {
			for(int n = 0; execution_count > n; ++n) {
				auto opts = execution_options_list[n];
				for(auto act_idx = 0; opts.actions_length > act_idx; ++act_idx) {
					auto& act = opts.actions[act_idx];
					exec_err = _validate_action(reg_id, act);
					if(exec_err != ECSACT_EXEC_SYS_OK) {
						return exec_err;
					}
				}
			}
		}

		for(int n = 0; execution_count > n; ++n) {
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
			_trigger_create_entity_event(info, *events_collector);
			_trigger_init_component_events(info, *events_collector);
			_trigger_update_component_events(info, *events_collector);
			_trigger_remove_component_events(info, *events_collector);
			_trigger_destroy_entity_event(info, *events_collector);
		}
		_clear_event_markers(info);

		info.mutex = std::nullopt;
		return exec_err;
	}
};

} // namespace ecsact::entt
