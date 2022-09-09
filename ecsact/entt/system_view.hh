#pragma once

#include <boost/mp11.hpp>
#include <entt/entt.hpp>
#include "ecsact/entt/event_markers.hh"

namespace ecsact::entt::detail {
	template<typename... C, typename... E>
	auto system_view_helper
		( boost::mp11::mp_list<C...>
		, boost::mp11::mp_list<E...>
		, ::entt::registry& registry
		)
	{
		return registry.view<C...>(::entt::exclude<E...>);
	}
}

namespace ecsact::entt {
	template<typename Package, typename SystemT>
	auto system_view
		( ::entt::registry& registry
		)
	{
		using boost::mp11::mp_unique;
		using boost::mp11::mp_push_back;
		using boost::mp11::mp_flatten;
		using boost::mp11::mp_list;
		using boost::mp11::mp_push_back;
		using boost::mp11::mp_assign;
		using boost::mp11::mp_transform;
		using boost::mp11::mp_map_find;

		using ecsact::entt::detail::temp_storage;
		using ecsact::entt::detail::beforechange_storage;

		using readonly_components = mp_map_find<
			typename Package::system_readonly_components,
			SystemT
		>;
		using readwrite_components = mp_map_find<
			typename Package::system_readwrite_components,
			SystemT
		>;
		using optional_components = mp_map_find<
			typename Package::system_optional_components,
			SystemT
		>;
		using adds_components = mp_map_find<
			typename Package::system_adds_components,
			SystemT
		>;
		using removes_components = mp_map_find<
			typename Package::system_removes_components,
			SystemT
		>;
		using include_components = mp_map_find<
			typename Package::system_include_components,
			SystemT
		>;
		using exclude_components = mp_map_find<
			typename Package::system_exclude_components,
			SystemT
		>;

		using get_types = mp_assign<mp_list<>, mp_unique<mp_flatten<mp_push_back<
			readonly_components,
			readwrite_components,
			include_components,
			mp_transform<beforechange_storage, readwrite_components>
		>>>>;

		using exclude_types = mp_assign<mp_list<>, mp_unique<mp_flatten<mp_push_back<
			exclude_components
		>>>>;

		return detail::system_view_helper(get_types{}, exclude_types{}, registry);
	}

	template<typename Package, typename SystemT>
	using system_view_type = decltype(
		system_view<Package, SystemT>(std::declval<::entt::registry&>())
	);
}
