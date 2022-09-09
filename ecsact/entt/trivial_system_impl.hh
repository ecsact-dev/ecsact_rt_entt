#pragma once

#include <boost/mp11.hpp>
#include "ecsact/entt/system_view.hh"

namespace ecsact::entt {

	template<typename Package, typename SystemT>
	constexpr bool is_trivial_system() {
		using boost::mp11::mp_map_find;
		using boost::mp11::mp_filter;
		using boost::mp11::mp_empty;
		using boost::mp11::mp_size;

		using readonly_components = mp_map_find<
			typename Package::system_readonly_components,
			SystemT
		>;
		using readwrite_components = mp_map_find<
			typename Package::system_readwrite_components,
			SystemT
		>;
		using writeonly_components = mp_map_find<
			typename Package::system_writeonly_components,
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
		using adds_tag_components = mp_filter<std::is_empty, adds_components>;

		const bool can_add_non_tag_compnents =
			mp_size<adds_components>::value != 0 && (
				mp_size<adds_tag_components>::value !=
				mp_size<adds_components>::value
			);

		const bool can_write =
			!mp_empty<readwrite_components>::value ||
			!mp_empty<writeonly_components>::value;

		const bool can_add_or_remove =
			!mp_empty<adds_components>::value ||
			!mp_empty<removes_components>::value;

		const bool can_meaningfully_read = can_add_or_remove && (
			!mp_empty<readonly_components>::value ||
			!mp_empty<readwrite_components>::value
		);
		
		return !can_write || !can_meaningfully_read || !can_add_non_tag_compnents;
	}

	template<typename Package, typename SystemT>
		requires (is_trivial_system<Package, SystemT>())
	void trivial_system_impl
		( ::entt::registry& registry
		)
	{
		using boost::mp11::mp_map_find;
		using boost::mp11::mp_empty;
		using boost::mp11::mp_for_each;

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

		constexpr bool is_removes_only =
			!mp_empty<removes_components>::value &&
			mp_empty<readonly_components>::value &&
			mp_empty<readwrite_components>::value &&
			mp_empty<optional_components>::value &&
			mp_empty<adds_components>::value &&
			mp_empty<include_components>::value &&
			mp_empty<exclude_components>::value;

		if constexpr(is_removes_only) {
			mp_for_each<removes_components>([&]<typename C>(C) {
				registry.clear<C>();
			});
		} else {
			auto view = system_view<Package, SystemT>(registry);

			for(auto entity : view) {
				mp_for_each<adds_components>([&]<typename C>(C) {
					registry.emplace<C>(entity);
				});

				mp_for_each<removes_components>([&]<typename C>(C) {
					registry.erase<C>(entity);
				});
			}
		}
	}

}
