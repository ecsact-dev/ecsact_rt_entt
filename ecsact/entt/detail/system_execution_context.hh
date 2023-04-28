#pragma once

#include <stdexcept>
#include <type_traits>
#include <string>
#include <cassert>
#include <optional>
#include <unordered_set>
#include <entt/entt.hpp>
#include "ecsact/runtime/common.h"
#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/entity.hh"

namespace ecsact::entt {
/**
 * Opaque index representing a system-likes association where `0` means no
 * association.
 */
enum class assoc_index : unsigned;

constexpr auto empty_assoc_index(assoc_index n) -> bool {
	return n == assoc_index{};
}

constexpr auto underlying_assoc_index(assoc_index n) -> unsigned {
	assert(!empty_assoc_index(n));
	return static_cast<unsigned>(n);
}

} // namespace ecsact::entt

struct ecsact_system_execution_context {
	ecsact_system_like_id   id;
	ecsact::entt::entity_id entity;
	::entt::registry*       registry = nullptr;
	const void*             action_data = nullptr;

	// pass in the context to this class that's a pointer
	// context(ptr) = parent_ctx(ptr)
	// no longer dependent on context ptr
	// parent_ctx made a copy of the pointer

	ecsact_system_execution_context* parent_ctx = nullptr;

	virtual ~ecsact_system_execution_context() = default;

	virtual auto action( //
		void* out_action_data
	) -> void = 0;

	virtual auto add( //
		ecsact_component_like_id component_id,
		const void*              component_data
	) -> void = 0;

	virtual auto remove( //
		ecsact_component_like_id component_id
	) -> void = 0;

	virtual auto get( //
		ecsact_component_like_id component_id,
		void*                    out_component_data
	) -> void = 0;

	virtual auto update( //
		ecsact_component_like_id component_id,
		const void*              component_data
	) -> void = 0;

	virtual auto has( //
		ecsact_component_like_id component_id
	) -> bool = 0;

	virtual auto generate( //
		int                  component_count,
		ecsact_component_id* component_ids,
		const void**         components_data
	) -> void = 0;

	virtual auto parent() -> const ecsact_system_execution_context* = 0;

	virtual auto other( //
		ecsact_entity_id entity
	) -> ecsact_system_execution_context* = 0;
};
