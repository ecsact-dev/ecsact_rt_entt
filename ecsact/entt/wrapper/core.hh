#pragma once

#include <cassert>
#include <type_traits>
#include "ecsact/runtime/common.h"
#include "ecsact/entt/detail/internal_markers.hh"
#include "ecsact/entt/event_markers.hh"
#include "entt/entity/registry.hpp"
#include "ecsact/entt/registry_util.hh"
#include "ecsact/entt/error_check.hh"
#include "ecsact/entt/detail/execution_events_collector.hh"

namespace ecsact::entt::wrapper::core {

template<typename C>
inline auto has_component( //
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id
) -> bool {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);
	return reg.all_of<C>(entity);
}

template<typename C>
inline auto get_component(
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id
) -> const void* {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	const C& comp = reg.get<C>(entity);
	return &comp;
}

template<typename C>
inline auto add_component( //
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id,
	const void*                          component_data
) -> ecsact_add_error {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	auto err = ecsact::entt::check_add_component_error<C>(
		reg,
		entity,
		*static_cast<const C*>(component_data)
	);

	if(err == ECSACT_ADD_OK) {
		if constexpr(std::is_empty_v<C>) {
			reg.emplace<C>(entity);
		} else {
			reg.emplace<detail::beforechange_storage<C>>(
				entity,
				*static_cast<const C*>(component_data),
				false
			);
			reg.emplace<C>(entity, *static_cast<const C*>(component_data));
		}

		ecsact::entt::detail::add_system_markers_if_needed<C>(reg, entity);
	}

	return err;
}

template<typename C>
inline auto add_component_exec_options( //
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id,
	const void*                          component_data
) -> ecsact_add_error {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	auto err = ecsact::entt::check_add_component_error<C>(
		reg,
		entity,
		*static_cast<const C*>(component_data)
	);

	assert(err == ECSACT_ADD_OK);

	if(err == ECSACT_ADD_OK) {
		if constexpr(std::is_empty_v<C>) {
			reg.emplace<C>(entity);
		} else {
			reg.emplace<detail::beforechange_storage<C>>(
				entity,
				*static_cast<const C*>(component_data)
			);
			reg.emplace<C>(entity, *static_cast<const C*>(component_data));
		}
		reg.template emplace_or_replace<component_added<C>>(entity);
		ecsact::entt::detail::add_system_markers_if_needed<C>(reg, entity);
	}

	return err;
}

template<typename C>
inline auto update_component( //
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id,
	const void*                          component_data
) -> ecsact_update_error {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);
	static_assert(!std::is_empty_v<C>, "Tag components cannot be updated");

	auto err = ecsact::entt::check_update_component_error<C>(
		reg,
		entity,
		*static_cast<const C*>(component_data)
	);

	if(err == ECSACT_UPDATE_OK) {
		reg.replace<C>(entity, *static_cast<const C*>(component_data));
	}

	return err;
}

template<typename C>
inline auto update_component_exec_options( //
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id,
	const void*                          component_data
) -> ecsact_update_error {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);
	static_assert(!std::is_empty_v<C>, "Tag components cannot be updated");

	auto err = ecsact::entt::check_update_component_error<C>(
		reg,
		entity,
		*static_cast<const C*>(component_data)
	);

	if(err == ECSACT_UPDATE_OK) {
		reg.replace<C>(entity, *static_cast<const C*>(component_data));
		reg.template emplace_or_replace<component_updated<C>>(entity);
	}

	return err;
}

template<typename C>
auto remove_component(
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id
) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	reg.remove<C>(entity);
	if constexpr(!std::is_empty_v<C>) {
		reg.remove<detail::beforechange_storage<C>>(entity);
	}
	reg.template remove<component_added<C>>(entity);
	reg.template remove<component_updated<C>>(entity);
	reg.template emplace_or_replace<component_removed<C>>(entity);
	ecsact::entt::detail::remove_system_markers_if_needed<C>(reg, entity);
}

template<typename C>
auto remove_component_exec_options(
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id
) -> void {
	using ecsact::entt::detail::pending_remove;

	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	if constexpr(!std::is_empty_v<C>) {
		reg.template emplace_or_replace<detail::beforeremove_storage<C>>(
			entity,
			reg.template get<C>(entity)
		);
	}

	reg.template erase<C>(entity);
	reg.template remove<component_added<C>>(entity);
	reg.template remove<component_updated<C>>(entity);
	reg.template emplace_or_replace<component_removed<C>>(entity);

	if constexpr(!std::is_empty_v<C>) {
		reg.template remove<detail::beforechange_storage<C>>(entity);
	}

	ecsact::entt::detail::remove_system_markers_if_needed<C>(reg, entity);
}

inline auto _trigger_create_entity_events(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	using ecsact::entt::detail::created_entity;

	auto& reg = ecsact::entt::get_registry(registry_id);

	if(events_collector.has_entity_created_callback()) {
		::entt::basic_view created_view{
			reg.template storage<created_entity>(),
		};

		for(ecsact::entt::entity_id entity : created_view) {
			events_collector.invoke_entity_created_callback(
				entity,
				created_view.template get<created_entity>(entity).placeholder_entity_id
			);
		}
	}

	reg.clear<created_entity>();
}

inline auto _trigger_destroy_entity_events(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	using ecsact::entt::detail::destroyed_entity;

	auto& reg = ecsact::entt::get_registry(registry_id);

	if(events_collector.has_entity_destroyed_callback()) {
		::entt::basic_view destroy_view{
			reg.template storage<destroyed_entity>(),
		};

		for(ecsact::entt::entity_id entity : destroy_view) {
			events_collector.invoke_entity_destroyed_callback(entity);
		}
	}

	reg.clear<destroyed_entity>();
}

template<typename C>
auto _trigger_init_component_event(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);

	if(!events_collector.has_init_callback()) {
		return;
	}

	if constexpr(C::transient) {
		return;
	}

	::entt::basic_view added_view{
		reg.template storage<C>(),
		reg.template storage<component_added<C>>(),
	};

	for(ecsact::entt::entity_id entity : added_view) {
		if constexpr(std::is_empty_v<C>) {
			events_collector.invoke_init_callback<C>(entity);
		} else {
			events_collector.invoke_init_callback<C>(
				entity,
				added_view.template get<C>(entity)
			);
		}
	}
}

template<typename C>
auto _trigger_update_component_event(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	using ecsact::entt::component_updated;
	using ecsact::entt::detail::beforechange_storage;

	if(!events_collector.has_update_callback()) {
		return;
	}

	auto& reg = ecsact::entt::get_registry(registry_id);
	if constexpr(!C::transient && !std::is_empty_v<C>) {
		::entt::basic_view changed_view{
			reg.template storage<C>(),
			reg.template storage<beforechange_storage<C>>(),
			reg.template storage<component_updated<C>>(),
		};

		for(ecsact::entt::entity_id entity : changed_view) {
			auto& before = changed_view.template get<beforechange_storage<C>>(entity);
			auto& current = changed_view.template get<C>(entity);
			before.has_update_occured = false;

			if(before.value != current) {
				events_collector.invoke_update_callback<C>(entity, current);
			}
		}
	}
}

template<typename C>
auto _trigger_remove_component_event(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);

	if(!events_collector.has_remove_callback()) {
		return;
	}

	if constexpr(C::transient) {
		return;
	}

	if constexpr(std::is_empty_v<C>) {
		::entt::basic_view removed_view{
			reg.template storage<component_removed<C>>(),
		};
		for(ecsact::entt::entity_id entity : removed_view) {
			events_collector.invoke_remove_callback<C>(entity);
		}
	} else {
		::entt::basic_view removed_view{
			reg.template storage<detail::beforeremove_storage<C>>(),
			reg.template storage<component_removed<C>>(),
		};
		for(ecsact::entt::entity_id entity : removed_view) {
			events_collector.invoke_remove_callback<C>(
				entity,
				removed_view.template get<detail::beforeremove_storage<C>>(entity).value
			);
		}

		reg.template clear<detail::beforeremove_storage<C>>();
	}
}

auto check_action_error_t(
	ecsact_registry_id registry_id,
	const void*        action_data
) -> ecsact_execute_systems_error;

template<typename A>
inline auto check_action_error(
	ecsact_registry_id registry_id,
	const void*        action_data
) -> ecsact_execute_systems_error {
	auto& reg = ecsact::entt::get_registry(registry_id);

	auto action = *static_cast<const A*>(action_data);
	auto result = ecsact::entt::check_action_error(reg, action);

	return result;
}

template<typename C>
inline auto clear_component(ecsact_registry_id registry_id) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);

	reg.clear<ecsact::entt::component_added<C>>();
	reg.clear<ecsact::entt::component_updated<C>>();
	reg.clear<ecsact::entt::component_removed<C>>();
}

template<typename S>
inline auto clear_notify_component(ecsact_registry_id registry_id) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);

	reg.clear<ecsact::entt::detail::run_system<S>>();
}

template<typename C>
inline auto prepare_component(ecsact_registry_id registry_id) -> void {
	using namespace ecsact::entt;

	auto& reg = ecsact::entt::get_registry(registry_id);

	reg.template storage<C>();
	reg.template storage<component_added<C>>();
	reg.template storage<component_removed<C>>();

	if constexpr(!std::is_empty_v<C>) {
		reg.storage<detail::beforechange_storage<C>>();
		reg.template storage<component_updated<C>>();
	}
}

} // namespace ecsact::entt::wrapper::core
