#pragma once

#include <boost/mp11.hpp>
#include <entt/entt.hpp>
#include "ecsact/lib.hh"
#include "ecsact/entt/detail/mp11_util.hh"
#include "ecsact/entt/event_markers.hh"

namespace ecsact::entt::detail {
	template<typename... C, typename... E>
	auto system_view_helper
		( ::ecsact::mp_list<C...>
		, ::ecsact::mp_list<E...>
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
		using ecsact::entt_mp11_util::mp_map_find_value_or;
		using boost::mp11::mp_unique;
		using boost::mp11::mp_flatten;
		using boost::mp11::mp_assign;
		using boost::mp11::mp_transform;
		using boost::mp11::mp_map_find;
		using boost::mp11::mp_push_back;

		using ecsact::entt::detail::temp_storage;
		using ecsact::entt::detail::beforechange_storage;

		using readonly_components = mp_map_find_value_or<
			typename Package::system_readonly_components,
			SystemT,
			::ecsact::mp_list<>
		>;
		using readwrite_components = mp_map_find_value_or<
			typename Package::system_readwrite_components,
			SystemT,
			::ecsact::mp_list<>
		>;
		using removes_components = mp_map_find_value_or<
			typename Package::system_removes_components,
			SystemT,
			::ecsact::mp_list<>
		>;
		using include_components = mp_map_find_value_or<
			typename Package::system_include_components,
			SystemT,
			::ecsact::mp_list<>
		>;
		using exclude_components = mp_map_find_value_or<
			typename Package::system_exclude_components,
			SystemT,
			::ecsact::mp_list<>
		>;

		using get_types = mp_unique<mp_flatten<
			mp_push_back<
				readonly_components,
				readwrite_components,
				include_components,
				mp_transform<beforechange_storage, readwrite_components>
			>,
			::ecsact::mp_list<>
		>>;

		using exclude_types = mp_unique<mp_flatten<
			mp_push_back<
				exclude_components,
				removes_components
			>,
			::ecsact::mp_list<>
		>>;

		return detail::system_view_helper(get_types{}, exclude_types{}, registry);
	}

	template<typename Package, typename SystemT>
	using system_view_type = decltype(
		system_view<Package, SystemT>(std::declval<::entt::registry&>())
	);
}
