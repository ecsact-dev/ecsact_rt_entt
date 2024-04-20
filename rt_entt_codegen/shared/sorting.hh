#pragma once

#include <type_traits>
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/meta.h"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen {

template<typename SystemLikeID>
	requires(std::is_same_v<std::remove_cvref_t<SystemLikeID>, ecsact_system_id> ||
					 std::is_same_v<std::remove_cvref_t<SystemLikeID>, ecsact_action_id>)
ECSACT_ALWAYS_INLINE auto system_needs_sorted_entities(
	SystemLikeID                      id,
	const ecsact_entt_system_details& details
) -> bool {
	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(id);
	auto needs_sorted_entities = false;

	if constexpr(std::is_same_v<
								 std::remove_cvref_t<SystemLikeID>,
								 ecsact_system_id>) {
		auto lazy_rate = ecsact_meta_get_lazy_iteration_rate(
			static_cast<ecsact_system_id>(sys_like_id)
		);

		if(lazy_rate > 0) {
			needs_sorted_entities = true;
		}
	}

	return needs_sorted_entities;
}
} // namespace ecsact::rt_entt_codegen
