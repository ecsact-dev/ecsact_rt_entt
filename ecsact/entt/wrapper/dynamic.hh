#pragma once

#include <cassert>
#include <type_traits>
#include "ecsact/entt/entity.hh"
#include "entt/entity/registry.hpp"
#include "ecsact/entt/registry_util.hh"
#include "ecsact/entt/error_check.hh"
#include "ecsact/entt/detail/internal_markers.hh"
#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/detail/system_execution_context.hh"

namespace ecsact::entt::wrapper::dynamic {

template<typename C>
auto context_add(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id,
	const void*                               component_data
) -> void {
	using ecsact::entt::component_added;
	using ecsact::entt::component_removed;
	using ecsact::entt::detail::beforeremove_storage;
	using ecsact::entt::detail::exec_beforechange_storage;
	using ecsact::entt::detail::pending_add;

	assert(ecsact_id_cast<ecsact_component_like_id>(C::id) == component_id);

	auto  entity = context->entity;
	auto& registry = *context->registry;

	if constexpr(std::is_empty_v<C>) {
		registry.template emplace_or_replace<pending_add<C>>(entity);
	} else {
		const C* component = static_cast<const C*>(component_data);
		registry.template emplace_or_replace<pending_add<C>>(entity, *component);
		registry.template remove<beforeremove_storage<C>>(entity);
	}

	if constexpr(!C::transient) {
		if(registry.template all_of<component_removed<C>>(entity)) {
			registry.template erase<component_removed<C>>(entity);
		} else {
			registry.template emplace_or_replace<component_added<C>>(entity);
		}
	}
}

template<typename C>
auto component_add_trivial(
	::entt::registry&       registry,
	ecsact::entt::entity_id entity_id
) -> void {
	using ecsact::entt::component_added;
	using ecsact::entt::component_removed;
	using ecsact::entt::detail::pending_add;

	registry.template emplace_or_replace<pending_add<C>>(entity_id);

	if constexpr(!C::transient) {
		if(registry.template all_of<component_removed<C>>(entity_id)) {
			registry.template erase<component_removed<C>>(entity_id);
		} else {
			registry.template emplace_or_replace<component_added<C>>(entity_id);
		}
	}
}

template<typename C>
auto context_remove(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id
) -> void {
	assert(ecsact_id_cast<ecsact_component_like_id>(C::id) == component_id);

	using ecsact::entt::component_removed;
	using ecsact::entt::component_updated;
	using ecsact::entt::detail::beforeremove_storage;
	using ecsact::entt::detail::pending_remove;

	auto  entity = context->entity;
	auto& registry = *context->registry;

	registry.template remove<component_added<C>>(entity);
	registry.template remove<component_updated<C>>(entity);
	registry.template emplace_or_replace<pending_remove<C>>(entity);
	registry.template emplace_or_replace<component_removed<C>>(entity);

	// Stop here (tag)
	if constexpr(!std::is_empty_v<C>) {
		auto component = registry.template get<C>(entity);

		auto& remove_storage =
			registry.template emplace_or_replace<beforeremove_storage<C>>(entity);

		remove_storage.value = component;
	}
}

template<typename C>
auto component_remove_trivial(
	::entt::registry&       registry,
	ecsact::entt::entity_id entity_id
) -> void {
	using ecsact::entt::component_removed;
	using ecsact::entt::component_updated;
	using ecsact::entt::detail::beforeremove_storage;
	using ecsact::entt::detail::pending_remove;

	registry.template remove<component_added<C>>(entity_id);
	registry.template remove<component_updated<C>>(entity_id);
	registry.template emplace_or_replace<pending_remove<C>>(entity_id);
	registry.template emplace_or_replace<component_removed<C>>(entity_id);

	if constexpr(!std::is_empty_v<C>) {
		auto component = registry.template get<C>(entity_id);

		auto& remove_storage =
			registry.template emplace_or_replace<beforeremove_storage<C>>(entity_id);

		remove_storage.value = component;
	}
}

template<typename C>
auto context_get(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id,
	void*                                     out_component_data
) -> void {
	auto        entity = context->entity;
	const auto& registry = *context->registry;

	assert(registry.template any_of<C>(entity));

	*static_cast<C*>(out_component_data) = registry.template get<C>(entity);
}

template<typename C>
auto context_update(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id,
	const void*                               in_component_data
) -> void {
	using ecsact::entt::component_updated;
	using ecsact::entt::detail::exec_beforechange_storage;
	// TODO(Kelwan): for remove, beforeremove_storage

	auto  entity = context->entity;
	auto& registry = *context->registry;

	const auto& in_component = *static_cast<const C*>(in_component_data);

	auto& current_component = registry.template get<C>(entity);
	auto& beforechange =
		registry.template get<exec_beforechange_storage<C>>(entity);
	if(!beforechange.has_update_occurred) {
		beforechange.value = current_component;
		beforechange.has_update_occurred = true;
		registry.template emplace_or_replace<component_updated<C>>(entity);
	}
	current_component = in_component;
}

template<typename C>
auto context_has(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id
) -> bool {
	auto  entity = context->entity;
	auto& registry = *context->registry;

	return registry.template any_of<C>(entity);
}

template<typename C>
auto context_generate_add(
	ecsact_system_execution_context* context,
	ecsact_component_id              component_id,
	const void*                      component_data,
	ecsact::entt::entity_id          entity
) -> void {
	using ecsact::entt::detail::pending_add;

	auto& registry = *context->registry;

	const auto& component = *static_cast<const C*>(component_data);
	registry.template emplace<pending_add<C>>(entity, component);
	detail::add_system_markers_if_needed<C>(registry, entity);
}

} // namespace ecsact::entt::wrapper::dynamic
