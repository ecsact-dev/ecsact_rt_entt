#pragma once

#include <boost/mp11.hpp>
#include "ecsact/cpp/type_info.hh"
#include "ecsact/entt/system_view.hh"

namespace ecsact::entt {

	template<typename T>
	using is_not_empty = boost::mp11::mp_not<std::is_empty<T>>;

	/**
	 * Checks if a system 'trivial' i.e. there is only a single possible
	 * meaningful implementation.
	 */
	template<typename SystemT>
	constexpr bool is_trivial_system() {
		using boost::mp11::mp_filter;
		using boost::mp11::mp_empty;
		using boost::mp11::mp_size;
		using boost::mp11::mp_list;

		using caps_info = ecsact::system_capabilities_info<SystemT>;

		using readonly_components = typename caps_info::readonly_components;
		using readwrite_components = typename caps_info::readwrite_components;
		using writeonly_components = typename caps_info::writeonly_components;
		using include_components = typename caps_info::include_components;
		using exclude_components = typename caps_info::exclude_components;
		using adds_components = typename caps_info::adds_components;
		using removes_components = typename caps_info::removes_components;
		using adds_tag_components = mp_filter<std::is_empty, adds_components>;
		using remove_tag_components = mp_filter<std::is_empty, removes_components>;
		using adds_non_tag_components = mp_filter<is_not_empty, adds_components>;

		const bool can_add = !mp_empty<adds_components>::value;
		const bool can_remove = !mp_empty<removes_components>::value;
		const bool can_add_non_tag = !mp_empty<adds_non_tag_components>::value;

		const bool can_only_add_tag =
			mp_size<adds_tag_components>::value == mp_size<adds_components>::value;

		const bool has_only_trivial_modifiers = can_remove || can_only_add_tag;
		const bool can_access =
			!mp_empty<readwrite_components>::value ||
			!mp_empty<readonly_components>::value ||
			!mp_empty<writeonly_components>::value;

		if(!can_access && has_only_trivial_modifiers) {
			return true;
		}

		const bool cant_write =
			mp_empty<writeonly_components>::value &&
			mp_empty<readwrite_components>::value;
		const bool has_modifiers = can_add || can_remove;

		if(cant_write && !has_modifiers) {
			return true;
		}
		
		return false;
	}

	template<typename SystemT, typename EachCallbackT>
		requires (is_trivial_system<SystemT>())
	void trivial_system_impl
		( auto&            info
		, EachCallbackT&&  each_callback = [](auto&, auto){}
		)
	{
		using boost::mp11::mp_for_each;

		using caps_info = ecsact::system_capabilities_info<SystemT>;
		using adds_components = typename caps_info::adds_components;
		using removes_components = typename caps_info::removes_components;
	
		auto view = system_view<SystemT>(info.registry);
		// TODO(zaucy): Iterate over association views in trivial systems
		auto assoc_views = system_association_views<SystemT>(info.registry);
		for(auto entity : view) {
			mp_for_each<adds_components>([&]<typename C>(C) {
				// Only empty components should have made it into this list if the
				// `is_trivial_system` constraint succeeded.
				static_assert(std::is_empty_v<C>);
				info.add_compnent<C>(entity);
			});

			mp_for_each<removes_components>([&]<typename C>(C) {
				info.remove_component<C>(entity);
			});

			each_callback(view, assoc_views, entity);
		}
	}

}
