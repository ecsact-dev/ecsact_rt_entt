#pragma once

#include <map>
#include <boost/mp11.hpp>
#include "ecsact/runtime/common.h"

namespace ecsact::entt::detail {

template<typename Package, typename Callback>
void mp_for_each_available_component(Callback&& cb) {
	using boost::mp11::mp_for_each;
	using components = typename Package::components;
	using dependencies = typename Package::dependencies;

	mp_for_each<components>([&]<typename C>(const C& c) { cb(c); });
	mp_for_each<dependencies>([&]<typename D>(const D& d) {
		mp_for_each_available_component<D>(cb);
	});
}

template<typename Package, typename Callback>
void mp_for_each_available_action(Callback&& cb) {
	using boost::mp11::mp_for_each;
	using actions = typename Package::actions;
	using dependencies = typename Package::dependencies;

	mp_for_each<actions>([&]<typename A>(const A& a) { cb(a); });
	mp_for_each<dependencies>([&]<typename D>(const D& d) {
		mp_for_each_available_action<D>(cb);
	});
}

template<typename Package, typename Callback>
void mp_for_each_available_system(Callback&& cb) {
	using boost::mp11::mp_for_each;
	using systems = typename Package::systems;
	using dependencies = typename Package::dependencies;

	mp_for_each<systems>([&]<typename S>(const S& s) { cb(s); });
	mp_for_each<dependencies>([&]<typename D>(const D& d) {
		mp_for_each_available_system<D>(cb);
	});
}

template<typename Package, typename Callback>
void mp_for_each_available_system_like(Callback&& cb) {
	mp_for_each_available_system<Package>(cb);
	mp_for_each_available_action<Package>(cb);
}

} // namespace ecsact::entt::detail
