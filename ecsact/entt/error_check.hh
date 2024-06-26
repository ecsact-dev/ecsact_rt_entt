#pragma once

#include <vector>
#include <unordered_map>
#include <cassert>
#include <span>
#include <type_traits>
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/core.h"
#include "entt/entity/registry.hpp"
#include "ecsact/entt/entity.hh"

namespace ecsact::entt::detail {
template<typename>
constexpr bool error_check_unimplemented_by_codegen = false;
}

namespace ecsact::entt {

template<typename C>
auto check_add_component_error( //
	ecsact::entt::registry_t&,
	::ecsact::entt::entity_id,
	const C&
) -> ecsact_add_error {
	static_assert(detail::error_check_unimplemented_by_codegen<C>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR                                                           |
| `check_add_component_error<>` template specialization cannot be found. This |
| is typically generated by ecsact_rt_entt_codegen.                           |
 -----------------------------------------------------------------------------
)");
}

template<typename C>
auto check_update_component_error( //
	ecsact::entt::registry_t&,
	::ecsact::entt::entity_id,
	const C&
) -> ecsact_update_error {
	static_assert(detail::error_check_unimplemented_by_codegen<C>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR                                                           |
| `check_update_component_error<>` template specialization cannot be found.   |
| This is typically generated by ecsact_rt_entt_codegen.                      |
 -----------------------------------------------------------------------------
)");
}

template<typename A>
auto check_action_error( //
	ecsact::entt::registry_t&,
	const A&
) -> ecsact_execute_systems_error {
	static_assert(detail::error_check_unimplemented_by_codegen<A>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR                                                           |
| `check_action_error<>` template specialization cannot be found.   |
| This is typically generated by ecsact_rt_entt_codegen.                      |
 -----------------------------------------------------------------------------
)");
}

} // namespace ecsact::entt
