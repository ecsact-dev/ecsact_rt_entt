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
auto system_view_helper(
	::ecsact::mp_list<C...>,
	::ecsact::mp_list<E...>,
	::entt::registry& registry
) {
	return registry.view<C...>(::entt::exclude<E...>);
}
} // namespace ecsact::entt::detail

namespace ecsact::entt {

template<
	typename SystemCapabilitiesInfo,
	typename ExtraGetTypes = ::ecsact::mp_list<>>
using view_get_types = boost::mp11::mp_unique<boost::mp11::mp_flatten<
	boost::mp11::mp_push_back<
		typename SystemCapabilitiesInfo::readonly_components,
		typename SystemCapabilitiesInfo::readwrite_components,
		typename SystemCapabilitiesInfo::include_components,
		typename SystemCapabilitiesInfo::removes_components,
		ExtraGetTypes,
		boost::mp11::mp_transform<
			detail::beforechange_storage,
			typename SystemCapabilitiesInfo::readwrite_components>>,
	::ecsact::mp_list<>>>;

template<
	typename SystemCapabilitiesInfo,
	typename ExtraExcludeTypes = ::ecsact::mp_list<>>
using view_exclude_types = boost::mp11::mp_unique<boost::mp11::mp_flatten<
	boost::mp11::mp_push_back<
		typename SystemCapabilitiesInfo::exclude_components,
		typename SystemCapabilitiesInfo::adds_components,
		ExtraExcludeTypes>,
	::ecsact::mp_list<>>>;

template<
	typename SystemCapabilitiesInfo,
	typename ExtraGetTypes = ::ecsact::mp_list<>,
	typename ExtraExcludeTypes = ::ecsact::mp_list<>>
auto view_from_system_capabilities(::entt::registry& registry) {
	return detail::system_view_helper(
		view_get_types<SystemCapabilitiesInfo, ExtraGetTypes>{},
		view_exclude_types<SystemCapabilitiesInfo, ExtraExcludeTypes>{},
		registry
	);
}

template<
	typename SystemCapabilitiesInfo,
	typename ExtraGetTypes = ::ecsact::mp_list<>,
	typename ExtraExcludeTypes = ::ecsact::mp_list<>>
using view_from_system_capabilities_type =
	decltype(view_from_system_capabilities<
					 SystemCapabilitiesInfo,
					 ExtraGetTypes,
					 ExtraExcludeTypes>(std::declval<::entt::registry&>()));

template<typename Assoc>
auto association_view(::entt::registry& registry) {
	using ecsact::entt::detail::association;
	using assoc_marker_component =
		association<typename Assoc::component_type, Assoc::field_offset>;
	using extra_get_types = ::ecsact::mp_list<assoc_marker_component>;

	return view_from_system_capabilities<Assoc, extra_get_types>(registry);
}

template<typename Assoc>
using association_view_type =
	decltype(association_view<Assoc>(std::declval<::entt::registry&>()));

template<typename SystemCapabilitiesInfo>
auto association_views(::entt::registry& registry) {
	using boost::mp11::mp_empty;
	using boost::mp11::mp_for_each;
	using boost::mp11::mp_rename;
	using boost::mp11::mp_transform;

	using caps_info = SystemCapabilitiesInfo;
	using assocs = typename caps_info::associations;

	using result_type = mp_rename<
		mp_transform<association_view_type, typename caps_info::associations>,
		std::tuple>;

	result_type result;

	mp_for_each<assocs>([&]<typename Assoc>(Assoc) {
		std::get<association_view_type<Assoc>>(result) =
			association_view<Assoc>(registry);
	});

	return result;
}

template<typename SystemCapabilitiesInfo>
using association_views_type =
	decltype(association_views<SystemCapabilitiesInfo>(
		std::declval<::entt::registry&>()
	));

template<typename SystemT>
auto system_view(::entt::registry& registry) {
	using caps_info = ecsact::system_capabilities_info<SystemT>;

	return view_from_system_capabilities<caps_info>(registry);
}

template<typename SystemT>
using system_view_type =
	decltype(system_view<SystemT>(std::declval<::entt::registry&>()));

template<typename SystemT>
auto system_association_views(::entt::registry& registry) {
	using caps_info = ecsact::system_capabilities_info<SystemT>;

	return association_views<caps_info>(registry);
}

template<typename SystemT>
using system_association_views_type =
	decltype(system_association_views<SystemT>(std::declval<::entt::registry&>())
	);

template<typename AssocViews>
auto system_association_views_iterators(AssocViews& assoc_views) {
	return std::apply(
		[&](auto&... views) { return std::make_tuple(views.begin()...); },
		assoc_views
	);
}

template<typename T>
struct system_or_association_view {
	using type = view_from_system_capabilities_type<T>;
};

template<ecsact::entt::detail::association_concept T>
struct system_or_association_view<T> {
	using type = association_view_type<T>;
};

template<typename T>
using system_or_association_view_t =
	typename system_or_association_view<T>::type;

} // namespace ecsact::entt
