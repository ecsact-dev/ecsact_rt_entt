#include "ecsact/entt/detail/allocator.hh"

using ecsact::entt::detail::static_allocator_base;

#define DEF_ALIGNED_STORAGE(n)                  \
	decltype(static_allocator_base::storage##n##_ \
	) static_allocator_base::storage##n##_ = {}

DEF_ALIGNED_STORAGE(1);
DEF_ALIGNED_STORAGE(4);
DEF_ALIGNED_STORAGE(8);
