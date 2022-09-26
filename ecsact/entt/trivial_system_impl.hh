#pragma once

#include <boost/mp11.hpp>
#include "ecsact/cpp/type_info.hh"
#include "ecsact/entt/system_view.hh"

namespace ecsact::entt {

	/**
	 * Checks if a system 'trivial' i.e. there is only a single possible
	 * meaningful implementation.
	 */
	template<typename Package, typename SystemT>
	constexpr bool is_trivial_system() {
		using boost::mp11::mp_filter;
		using boost::mp11::mp_empty;
		using boost::mp11::mp_size;
		using boost::mp11::mp_list;

		using caps_info = ecsact::system_capabilities_info<SystemT>;

		using readonly_components = typename caps_info::readonly_components;
		using readwrite_components = typename caps_info::readwrite_components;
		using writeonly_components = typename caps_info::writeonly_components;
		using adds_components = typename caps_info::adds_components;
		using removes_components = typename caps_info::removes_components;
		using adds_tag_components = mp_filter<std::is_empty, adds_components>;

		const bool can_add_non_tag_compnents =
			mp_size<adds_components>::value != 0 && (
				mp_size<adds_tag_components>::value !=
				mp_size<adds_components>::value
			);
		const bool cant_write =
			mp_empty<readwrite_components>::value &&
			mp_empty<writeonly_components>::value;
		const bool cant_add = mp_empty<adds_components>::value;
		const bool cant_remove = mp_empty<removes_components>::value;
		const bool cant_read =
			mp_empty<readonly_components>::value &&
			mp_empty<readwrite_components>::value;
		
		return cant_write && cant_add && cant_remove && cant_read;
	}

	template<typename Package, typename SystemT, typename EachCallbackT>
		requires (is_trivial_system<Package, SystemT>())
	void trivial_system_impl
		( ::entt::registry&  registry
		, EachCallbackT&&    each_callback = [](auto&, auto){}
		)
	{
		using boost::mp11::mp_empty;
		using boost::mp11::mp_list;
		using boost::mp11::mp_for_each;

		using caps_info = ecsact::system_capabilities_info<SystemT>;

		using readonly_components = typename caps_info::readonly_components;
		using readwrite_components = typename caps_info::readwrite_components;
		using writeonly_components = typename caps_info::writeonly_components;
		using adds_components = typename caps_info::adds_components;
		using removes_components = typename caps_info::removes_components;
		using include_components = typename caps_info::include_components;
		using exclude_components = typename caps_info::exclude_components;
		using optional_components = typename caps_info::optional_components;

		// If we have a system that can only remove and does not use any filtering
		// i.e. simply removes all of a component, then we can use a short cut and
		// use `registry.clear`.
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
					// Only empty comopnents should have made it into this list if the
					// `is_trivial_system` constraint succeeded.
					static_assert(std::is_empty_v<C>);
					registry.emplace<C>(entity);
				});

				mp_for_each<removes_components>([&]<typename C>(C) {
					registry.erase<C>(entity);
				});

				each_callback(view, entity);
			}
		}
	}

}
