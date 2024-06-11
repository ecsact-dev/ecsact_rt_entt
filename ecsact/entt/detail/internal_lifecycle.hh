#pragma once

#include <type_traits>
#include "entt/entt.hpp"
#include "ecsact/entt/entity.hh"
#include "ecsact/runtime/common.h"

namespace ecsact::entt::detail {

template<typename>
constexpr bool lifecycle_unimplemented_by_codegen = false;

template< typename C>
  requires(!std::is_empty_v<C>)
auto lifecycle_on_add( //
	ecsact::entt::registry_t&,
	ecsact::entt::entity_id,
  const C&
) -> void {
	static_assert(lifecycle_unimplemented_by_codegen<C>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR - missing specialization                                  |
 -----------------------------------------------------------------------------
)");
}

template<typename C>
	requires(std::is_empty_v<C>)
auto lifecycle_on_add( //
	ecsact::entt::registry_t&,
	ecsact::entt::entity_id
) -> void {
	static_assert(lifecycle_unimplemented_by_codegen<C>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR - missing specialization                                  |
 -----------------------------------------------------------------------------
)");
}

template<typename C>
auto lifecycle_on_update(
	ecsact::entt::registry_t&,
	ecsact::entt::entity_id,
	const C& before,
	const C& after
) -> void {
	static_assert(lifecycle_unimplemented_by_codegen<C>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR - missing specialization                                  |
 -----------------------------------------------------------------------------
)");
}
} // namespace ecsact::entt::detail
