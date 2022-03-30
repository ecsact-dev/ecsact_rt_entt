#pragma once

#include <type_traits>

namespace ecsact::entt {
	/**
	 * Marker to indicate that a component has been added
	 */
	template<typename Component>
	struct component_added {};

	/**
	 * Marker to indicate that a component has been changed during execution
	 */
	template<typename Component>
	struct component_changed {};

	/**
	 * Marker to indicate that a component has been removed
	 */
	template<typename Component>
	struct component_removed {};
}

namespace ecsact::entt::detail {
	template<typename C>
	struct temp_storage;

	template<typename C> requires(std::is_empty_v<C>)
	struct temp_storage<C> { };

	template<typename C> requires(!std::is_empty_v<C>)
	struct temp_storage<C> { C value; };
	
	template<typename C>
	struct beforechange_storage;

	template<typename C> requires(std::is_empty_v<C>)
	struct beforechange_storage<C> { };

	template<typename C> requires(!std::is_empty_v<C>)
	struct beforechange_storage<C> { C value; };
}
