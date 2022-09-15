#pragma once

#include <entt/entt.hpp>

namespace ecsact_entt_rt {
  inline void ensure_entity
    ( entt::registry&  registry
    , entt::entity     entity
    )
  {
    if(!registry.valid(entity)) {
      static_cast<void>(registry.create(entity));
      assert(registry.valid(entity));
    }
  }

  template<typename C>
  bool has_entity_and_component
    ( entt::registry&  registry
    , entt::entity     entity
    )
  {
    return registry.valid(entity) && registry.all_of<C>(entity);
  }

}
