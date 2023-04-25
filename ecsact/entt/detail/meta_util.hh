#pragma once

#include "ecsact/runtime/common.h"
#include "ecsact/lib.hh"
#include <boost/mp11.hpp>
#include <map>

namespace ecsact::entt::detail {

template<typename... Package>
struct mp_components_t;

template<>
struct mp_components_t<> {
	using type = ::ecsact::mp_list<>;
};

template<typename Package>
struct mp_components_t<Package> {
	using type = typename Package::components;
};

template<typename HeadPackage, typename... Package>
struct mp_components_t<HeadPackage, Package...> {
	using type =
		boost::mp11::mp_unique<boost::mp11::mp_flatten<boost::mp11::mp_push_back<
			typename mp_components_t<HeadPackage>::type,
			typename mp_components_t<Package...>::type>>>;
};

template<typename... Package>
struct mp_transients_t;

template<>
struct mp_transients_t<> {
	using type = ::ecsact::mp_list<>;
};

template<typename Package>
struct mp_transients_t<Package> {
	using type = typename Package::transients;
};

template<typename HeadPackage, typename... Package>
struct mp_transients_t<HeadPackage, Package...> {
	using type =
		boost::mp11::mp_unique<boost::mp11::mp_flatten<boost::mp11::mp_push_back<
			typename mp_transients_t<HeadPackage>::type,
			typename mp_transients_t<Package...>::type>>>;
};

template<typename... Package>
struct mp_actions_t;

template<>
struct mp_actions_t<> {
	using type = ::ecsact::mp_list<>;
};

template<typename Package>
struct mp_actions_t<Package> {
	using type = typename Package::actions;
};

template<typename HeadPackage, typename... Package>
struct mp_actions_t<HeadPackage, Package...> {
	using type =
		boost::mp11::mp_unique<boost::mp11::mp_flatten<boost::mp11::mp_push_back<
			typename mp_actions_t<HeadPackage>::type,
			typename mp_actions_t<Package...>::type>>>;
};

template<typename... Package>
struct mp_systems_t;

template<>
struct mp_systems_t<> {
	using type = ::ecsact::mp_list<>;
};

template<typename Package>
struct mp_systems_t<Package> {
	using type = typename Package::systems;
};

template<typename HeadPackage, typename... Package>
struct mp_systems_t<HeadPackage, Package...> {
	using type =
		boost::mp11::mp_unique<boost::mp11::mp_flatten<boost::mp11::mp_push_back<
			typename mp_systems_t<HeadPackage>::type,
			typename mp_systems_t<Package...>::type>>>;
};

template<typename Package>
using mp_package_dependencies = typename Package::dependencies;

template<typename PackageList>
using mp_package_dependencies_from_list = std::enable_if_t<
	!boost::mp11::mp_empty<PackageList>::value,
	boost::mp11::mp_flatten<
		boost::mp11::mp_transform<mp_package_dependencies, PackageList>>>;

/**
 * Given a list of packages return a list of all unique dependencies recursively
 */
template<typename PackageList>
using mp_package_dependencies_recursive =
	boost::mp11::mp_unique<boost::mp11::mp_apply<
		boost::mp11::mp_append,
		boost::mp11::mp_iterate<
			PackageList,
			boost::mp11::mp_identity_t,
			mp_package_dependencies_from_list>>>;

template<typename Package>
using mp_all_components_t = typename boost::mp11::mp_append<
	mp_components_t<Package>,
	mp_package_dependencies_recursive<typename Package::dependencies>>::type;

template<typename Package>
using mp_all_transients_t = typename boost::mp11::mp_append<
	mp_transients_t<Package>,
	mp_package_dependencies_recursive<typename Package::dependencies>>::type;

template<typename Package>
using mp_all_systems_t = typename boost::mp11::mp_append<
	mp_systems_t<Package>,
	mp_package_dependencies_recursive<typename Package::dependencies>>::type;

template<typename Package>
using mp_all_actions_t = typename boost::mp11::mp_append<
	mp_actions_t<Package>,
	mp_package_dependencies_recursive<typename Package::dependencies>>::type;

template<typename Package, typename Callback>
void mp_for_each_available_component(Callback&& cb) {
	using boost::mp11::mp_append;
	using boost::mp11::mp_for_each;

	using components = typename mp_append<
		mp_components_t<Package>,
		mp_package_dependencies_recursive<typename Package::dependencies>>::type;

	mp_for_each<components>([&]<typename C>(const C& c) { cb(c); });
}

template<typename Package, typename Callback>
void mp_for_each_available_action(Callback&& cb) {
	using boost::mp11::mp_append;
	using boost::mp11::mp_for_each;

	using actions = typename mp_append<
		mp_actions_t<Package>,
		mp_package_dependencies_recursive<typename Package::dependencies>>::type;

	mp_for_each<actions>([&]<typename C>(const C& c) { cb(c); });
}

template<typename Package, typename Callback>
void mp_for_each_available_system(Callback&& cb) {
	using boost::mp11::mp_append;
	using boost::mp11::mp_for_each;

	using systems = typename mp_append<
		mp_systems_t<Package>,
		mp_package_dependencies_recursive<typename Package::dependencies>>::type;

	mp_for_each<systems>([&]<typename C>(const C& c) { cb(c); });
}

template<typename Package, typename Callback>
void mp_for_each_available_system_like(Callback&& cb) {
	mp_for_each_available_system<Package>(cb);
	mp_for_each_available_action<Package>(cb);
}

} // namespace ecsact::entt::detail
