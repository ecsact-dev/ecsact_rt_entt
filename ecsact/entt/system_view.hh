#pragma once

#include <boost/mp11.hpp>
#include <entt/entt.hpp>
#include "ecsact/lib.hh"
#include "ecsact/entt/detail/mp11_util.hh"
#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/detail/internal_markers.hh"
#include "ecsact/cpp/type_info.hh"

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
	template<typename SystemT>
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

		using caps_info = ecsact::system_capabilities_info<SystemT>;

		using readonly_components = typename caps_info::readonly_components;
		using readwrite_components = typename caps_info::readwrite_components;
		using removes_components = typename caps_info::removes_components;
		using include_components = typename caps_info::include_components;
		using exclude_components = typename caps_info::exclude_components;

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

	template<typename SystemT>
	using system_view_type = decltype(
		system_view<SystemT>(std::declval<::entt::registry&>())
	);
}
