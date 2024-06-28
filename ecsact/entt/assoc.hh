#pragma once

#include <cstddef>

namespace ecsact::entt {

/**
 * This struct is used to tag an entity as 'indexed'. The template parameter @tp
 * C is only to make the code generation more clear which component is being
 * indexed. Since the storage name is the identifying factor it should have no
 * effect.
 */
template<typename C>
struct indexed {};

} // namespace ecsact::entt
