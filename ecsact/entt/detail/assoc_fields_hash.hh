#pragma once

#include <cstdarg>
#include <cstdint>
#include "entt/entt.hpp"
#include "ecsact/runtime/common.h"

namespace ecsact::entt::detail {
using assoc_hash_value_t = std::uint32_t;
static_assert(
	sizeof(::entt::id_type) == sizeof(assoc_hash_value_t),
	"EnTT storage id type must match the size of ecsact_rt_entt internal hash "
	"algorithm size"
);

auto get_assoc_fields_hash(
	ecsact_composite_id compo_id,
	std::va_list        indexed_field_values
) -> assoc_hash_value_t;

template<typename C>
	requires(C::has_assoc_fields)
auto get_assoc_fields_hash(const C&) -> assoc_hash_value_t;
} // namespace ecsact::entt::detail
