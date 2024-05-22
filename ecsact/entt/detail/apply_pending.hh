#pragma once

#include <entt/entt.hpp>
#include "ecsact/entt/wrapper/core.hh"
#include "ecsact/entt/detail/internal_markers.hh"

namespace ecsact::entt::detail {

template<typename C>
auto apply_pending_add(::entt::registry& registry) -> void {
	if constexpr(std::is_empty_v<C>) {
		registry.view<pending_add<C>>().each([&](auto entity) {
			registry.emplace<C>(entity);
		});
	} else {
		registry.view<pending_add<C>>().each(
			[&](auto entity, const pending_add<C>& comp) {
				registry.emplace<C>(entity, comp.value);
				registry.emplace<beforechange_storage<C>>(entity, comp.value, false);
			}
		);
	}
	registry.clear<pending_add<C>>();
}

template<typename C>
auto apply_pending_remove(::entt::registry& registry) -> void {
	registry.view<pending_remove<C>>().each([&](auto entity) {
		if constexpr(!std::is_empty_v<C>) {
			registry.erase<beforechange_storage<C>>(entity);
		}
		registry.erase<C>(entity);
	});
	registry.clear<pending_remove<C>>();
}

} // namespace ecsact::entt::detail
