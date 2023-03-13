#pragma once

#include <map>
#include <boost/mp11.hpp>
#include "ecsact/runtime/common.h"

namespace ecsact::entt::detail {

/**
 *
 */
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
		mp_for_each_available_components<D>(cb);
	});
}

} // namespace ecsact::entt::detail
