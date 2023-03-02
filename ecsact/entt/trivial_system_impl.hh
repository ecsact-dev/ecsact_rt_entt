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
	using boost::mp11::mp_empty;
	using boost::mp11::mp_filter;
	using boost::mp11::mp_list;
	using boost::mp11::mp_size;

	using caps_info = ecsact::system_capabilities_info<SystemT>;

	using readonly_components = typename caps_info::readonly_components;
	using readwrite_components = typename caps_info::readwrite_components;
	using writeonly_components = typename caps_info::writeonly_components;
	using adds_components = typename caps_info::adds_components;
	using removes_components = typename caps_info::removes_components;
	using adds_tag_components = mp_filter<std::is_empty, adds_components>;

	const bool can_add = !mp_empty<adds_components>::value;
	const bool can_remove = !mp_empty<removes_components>::value;

	const bool can_only_add_tag = mp_size<adds_tag_components>::value ==
		mp_size<adds_components>::value;

	const bool has_only_trivial_modifiers = can_remove || can_only_add_tag;
	const bool can_access = !mp_empty<readwrite_components>::value ||
		!mp_empty<readonly_components>::value ||
		!mp_empty<writeonly_components>::value;

	if(!can_access && has_only_trivial_modifiers) {
		return true;
	}

	const bool cant_write = mp_empty<writeonly_components>::value &&
		mp_empty<readwrite_components>::value;
	const bool has_modifiers = can_add || can_remove;

	if(cant_write && !has_modifiers) {
		return true;
	}

	return false;
}

template<typename SystemT, typename EachCallbackT>
	requires(is_trivial_system<SystemT>())
void trivial_system_impl(
	auto&           info,
	EachCallbackT&& each_callback = [](auto&, auto&, auto) {}
) {
	using boost::mp11::mp_for_each;
	using ecsact::entt::component_removed;
	using ecsact::entt::detail::pending_remove;
	using ecsact::entt::detail::temp_storage;

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
			info.template add_component<C>(entity);
		});

		mp_for_each<removes_components>([&]<typename C>(C) {
			info.registry.template emplace<pending_remove<C>>(entity);

			if constexpr(!C::transient) {
				if(info.registry.template all_of<component_added<C>>(entity)) {
					info.registry.template remove<component_added<C>>(entity);
					info.registry.template remove<component_removed<C>>(entity);
				} else {
					info.registry.template emplace_or_replace<component_removed<C>>(entity
					);
				}

				if constexpr(!std::is_empty_v<C>) {
					auto& temp = info.registry.template storage<temp_storage<C>>();
					if(temp.contains(entity)) {
						temp.get(entity).value = info.registry.template get<C>(entity);
					} else {
						temp.emplace(entity, info.registry.template get<C>(entity));
					}
				}
			}
		});

		each_callback(view, assoc_views, entity);
	}
}

} // namespace ecsact::entt
