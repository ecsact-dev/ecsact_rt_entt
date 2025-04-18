#pragma once

#include <type_traits>
#include "ecsact/runtime/common.hh"
#include "ecsact/entt/entity.hh"
#include "ecsact/entt/detail/registry.hh"
#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/detail/internal_markers.hh"

namespace ecsact::entt::detail {

template<typename C>
struct deferred_storage {
	template<template<typename> typename WrapperT>
	using storage_for = ::entt::storage_for_t<WrapperT<C>>;

	ecsact::entt::registry_t& registry;

	auto add() -> storage_for<pending_add>;
	auto remove() -> storage_for<pending_remove>;
};

template<typename C>
struct event_storage {
	template<template<typename> typename WrapperT>
	using storage_for = ::entt::storage_for_t<WrapperT<C>>;

	ecsact::entt::registry_t& registry;

	auto removed() -> storage_for<component_removed>;
	auto added() -> storage_for<component_added>;

	auto beforeremove() -> storage_for<beforeremove_storage>;
};

struct storage {
	template<typename C>
	using storage_for = ::entt::storage_for_t<C>;

	ecsact::entt::registry_t& registry;

	/**
	 * Simply the container where an Ecsact component is stored
	 */
	template<typename C>
	auto component() -> storage_for<C>;

	/**
	 * Containers responsible aiding with 'deferred' operations
	 */
	template<typename C>
	auto deferred() -> deferred_storage<C> {
		return deferred_storage<C>{registry};
	}

	template<typename C>
	auto event() -> event_storage<C> {
		return event_storage<C>{registry};
	}
};

/**
 * Update component in storage with value if exists. If doesn't exist, add one
 * with value.
 */
template<typename C>
	requires(!std::is_empty_v<C>)
auto ensure_component(
	::entt::storage_for_t<C> storage,
	entity_id                entity,
	const C&                 value
) -> C& {
}

template<typename C>
	requires(std::is_empty_v<C>)
auto ensure_component( //
	::entt::storage_for_t<C> storage,
	entity_id                entity
) -> void {
}

template<typename C>
auto remove_component( //
	::entt::storage_for_t<C> storage,
	entity_id                entity
) -> void {
}

template<typename C>
auto remove_component_unchecked(
	::entt::storage_for_t<C> storage,
	entity_id                entity
) -> void {
}

template<typename C>
	requires(!std::is_empty_v<C>)
auto add_component_unchecked( //
	::entt::storage_for_t<C> storage,
	entity_id                entity
) -> void {
}

template<typename C>
	requires(!std::is_empty_v<C>)
auto add_component_unchecked(
	::entt::storage_for_t<C> storage,
	entity_id                entity,
	const C&                 component
) -> void {
}

template<typename C>
auto has_component( //
	::entt::storage_for_t<C> storage,
	entity_id                entity
) -> bool {
}
} // namespace ecsact::entt::detail
