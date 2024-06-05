#pragma once

#include "entt/entity/registry.hpp"
#include "ecsact/entt/detail/allocator.hh"

namespace ecsact::entt {

using registry_t = ::entt::
	basic_registry<::entt::entity, detail::static_allocator<::entt::entity>>;
}

// using registry_t = ::entt::
// 	basic_registry<::entt::entity, detail::static_allocator<::entt::entity>>;
// }
