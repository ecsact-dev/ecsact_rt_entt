#pragma once

#include <cstdint>
#include <type_traits>
#include <concepts>
#include "ecsact/runtime/common.h"

namespace ecsact::entt::detail {

template<typename C, std::size_t FieldOffset>
struct association {
	using component = C;
	static constexpr auto field_offset = FieldOffset;
	std::int_fast16_t     ref_count = 0;
};

template<typename Assoc>
concept association_concept = //
	requires {
		{ typename Assoc::component_type{} };
		{ Assoc::field_offset } -> std::convertible_to<std::size_t>;
	};

template<typename C>
struct beforeremove_storage;

template<typename C>
	requires(std::is_empty_v<C>)
struct beforeremove_storage<C> {};

template<typename C>
	requires(!std::is_empty_v<C>)
struct beforeremove_storage<C> {
	C value;
};

template<typename C>
	requires(!std::is_empty_v<C>)
struct beforechange_storage {
	C value;
};

template<typename C>
struct pending_add;

template<typename C>
	requires(std::is_empty_v<C>)
struct pending_add<C> {};

template<typename C>
	requires(!std::is_empty_v<C>)
struct pending_add<C> {
	C value;
};

template<typename C>
struct pending_remove {};

struct created_entity {
	ecsact_placeholder_entity_id placeholder_entity_id;
};

struct destroyed_entity {};

template<typename S>
struct system_sorted;

} // namespace ecsact::entt::detail
