#pragma once

#include <cstdarg>
#include <cstdint>
#include "ecsact/runtime/common.h"

namespace ecsact::entt::detail {
auto get_assoc_fields_hash(
	ecsact_composite_id compo_id,
	std::va_list        indexed_field_values
) -> std::uint64_t;

template<typename C>
	requires(C::has_assoc_fields)
auto get_assoc_fields_hash(const C&) -> std::uint64_t;
} // namespace ecsact::entt::detail
