#pragma once

#include <boost/mp11.hpp>

namespace ecsact::entt_mp11_util {

template<typename M, typename K, typename V>
using mp_map_find_or = boost::mp11::mp_if<
	boost::mp11::mp_map_contains<M, K>,
	boost::mp11::mp_map_find<M, K>,
	boost::mp11::mp_list<K, V>>;

template<typename M, typename K, typename V>
using mp_map_find_value_or = boost::mp11::mp_second<mp_map_find_or<M, K, V>>;

} // namespace ecsact::entt_mp11_util
