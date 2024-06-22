#pragma once

#include <cstdarg>
#include <cstdint>
#include "ecsact/runtime/common.h"

namespace ecsact::entt::detail {
auto get_assoc_fields_hash(
	ecsact_composite_id compo_id,
	std::va_list        indexed_field_values
) -> std::uint64_t;
}
