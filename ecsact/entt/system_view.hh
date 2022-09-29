#pragma once

#include <tuple>
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

	template<typename SystemCapabilitiesInfo>
	auto view_from_system_capabilities
		( ::entt::registry& registry
		)
	{
		using caps_info = SystemCapabilitiesInfo;

		using ecsact::entt_mp11_util::mp_map_find_value_or;
		using boost::mp11::mp_unique;
		using boost::mp11::mp_flatten;
		using boost::mp11::mp_assign;
		using boost::mp11::mp_transform;
		using boost::mp11::mp_map_find;
		using boost::mp11::mp_push_back;

		using ecsact::entt::detail::temp_storage;
		using ecsact::entt::detail::beforechange_storage;

		using readonly_components = typename caps_info::readonly_components;
		using readwrite_components = typename caps_info::readwrite_components;
		using removes_components = typename caps_info::removes_components;
		using adds_components = typename caps_info::adds_components;
		using include_components = typename caps_info::include_components;
		using exclude_components = typename caps_info::exclude_components;

		using get_types = mp_unique<mp_flatten<
			mp_push_back<
				readonly_components,
				readwrite_components,
				include_components,
				removes_components,
				mp_transform<beforechange_storage, readwrite_components>
			>,
			::ecsact::mp_list<>
		>>;

		using exclude_types = mp_unique<mp_flatten<
			mp_push_back<
				exclude_components,
				adds_components
			>,
			::ecsact::mp_list<>
		>>;

		return detail::system_view_helper(get_types{}, exclude_types{}, registry);
	}

	template<typename SystemCapabilitiesInfo>
	using view_from_system_capabilities_type = decltype(
		view_from_system_capabilities<SystemCapabilitiesInfo>(
			std::declval<::entt::registry&>()
		)
	);

	template<typename SystemCapabilitiesInfo>
	auto association_views
		( ::entt::registry& registry
		)
	{
		using boost::mp11::mp_for_each;
		using boost::mp11::mp_transform;
		using boost::mp11::mp_rename;

		using caps_info = SystemCapabilitiesInfo;

		using result_type = mp_rename<
			mp_transform<
				view_from_system_capabilities_type,
				typename caps_info::associations
			>,
			std::tuple
		>;

		static_assert(
			boost::mp11::mp_size<typename caps_info::associations>::value ==
			std::tuple_size_v<result_type>,
			"[INTERNAL] result_type failure"
		);

		result_type result;

		mp_for_each<typename caps_info::associations>([&]<typename Assoc>(Assoc) {
			using view_type = view_from_system_capabilities_type<Assoc>;
			std::get<view_type>(result) =
				view_from_system_capabilities<Assoc>(registry);
		});

		return result;
	}

	template<typename SystemCapabilitiesInfo>
	using association_views_type = decltype(
		association_views<SystemCapabilitiesInfo>(std::declval<::entt::registry&>())
	);

	template<typename SystemT>
	auto system_view
		( ::entt::registry& registry
		)
	{
		using caps_info = ecsact::system_capabilities_info<SystemT>;

		return view_from_system_capabilities<caps_info>(registry);
	}

	template<typename SystemT>
	using system_view_type = decltype(
		system_view<SystemT>(std::declval<::entt::registry&>())
	);

	template<typename SystemT>
	auto system_association_views
		( ::entt::registry& registry
		)
	{
		using caps_info = ecsact::system_capabilities_info<SystemT>;

		return association_views<caps_info>(registry);
	}

	template<typename SystemT>
	using system_association_views_type = decltype(
		system_association_views<SystemT>(std::declval<::entt::registry&>())
	);

	template<typename AssocViews>
	auto system_association_views_iterators
		( AssocViews& assoc_views
		)
	{
		return std::apply([&](auto&... views) {
			return std::make_tuple(views.begin()...);
		}, assoc_views);
	}
}
