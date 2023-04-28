#pragma once

#include <stdexcept>
#include <type_traits>
#include <string>
#include <cassert>
#include <optional>
#include <unordered_set>
#include <boost/mp11.hpp>
#include <entt/entt.hpp>
#include "ecsact/runtime/common.h"
#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/system_view.hh"
#include "ecsact/entt/detail/meta_util.hh"

#include "registry_info.hh"

namespace ecsact_entt_rt {
struct system_execution_context_base;
}

struct ecsact_system_execution_context {
	ecsact_system_like_id system_id;
	// System execution context implementation. To be casted to specific derived
	// templated type. See `system_execution_context<Package, System>`
	ecsact_entt_rt::system_execution_context_base* impl;

	// This is set by the system_execution_context::other method. `0` means
	// no association.
	unsigned association_index = 0;
};

namespace ecsact_entt_rt {

struct system_execution_context_base {
	using cptr_t = struct ::ecsact_system_execution_context*;
	using const_cptr_t = const struct ::ecsact_system_execution_context*;

	ecsact::entt::entity_id entity;
	const cptr_t            parent;
	const void*             action;

	system_execution_context_base(
		::entt::entity entity,
		cptr_t         parent,
		const void*    action
	)
		: entity(entity), parent(parent), action(action) {
	}

	virtual ~system_execution_context_base() = default;

	virtual auto other( //
		ecsact::entt::entity_id other_entity
	) -> ecsact_system_execution_context* = 0;

	virtual auto generate(
		int                  component_count,
		ecsact_component_id* component_ids,
		const void**         components_data
	) -> void = 0;
};

namespace detail {}

template<typename Package, typename SystemCapabilitiesInfo>
struct system_execution_context : system_execution_context_base {
	using system_execution_context_base::const_cptr_t;
	using system_execution_context_base::cptr_t;

	template<typename T>
	using other_system_execution_context =
		std::optional<system_execution_context<Package, T>>;

	using caps_info = SystemCapabilitiesInfo;
	using package = Package;
	using view_type = ecsact::entt::system_or_association_view_t<caps_info>;
	using association_views_type =
		ecsact::entt::association_views_type<caps_info>;

	using readonly_components = typename caps_info::readonly_components;
	using readwrite_components = typename caps_info::readwrite_components;
	using adds_components = typename caps_info::adds_components;
	using removes_components = typename caps_info::removes_components;

	using gettable_components = boost::mp11::mp_assign<
		::ecsact::mp_list<>,
		boost::mp11::mp_unique<boost::mp11::mp_flatten<
			boost::mp11::mp_push_back<readonly_components, readwrite_components>,
			::ecsact::mp_list<>>>>;

	static_assert(
		boost::mp11::mp_size<typename caps_info::associations>::value ==
			std::tuple_size_v<association_views_type>,
		"Invalid associations or association views type"
	);

	ecsact_entt_rt::registry_info<Package>& info;
	view_type&                              view;
	association_views_type&                 assoc_views;

	using others_t = boost::mp11::mp_transform<
		other_system_execution_context,
		boost::mp11::mp_rename<typename caps_info::associations, std::tuple>>;

	others_t others;

	/** @internal */
	ecsact_system_execution_context _c_ctx;

	system_execution_context(
		ecsact_entt_rt::registry_info<Package>& info,
		ecsact_system_like_id                   system_id,
		view_type&                              view,
		association_views_type&                 assoc_views,
		::entt::entity                          entity,
		const cptr_t                            parent,
		const void*                             action
	)
		: system_execution_context_base(entity, parent, action)
		, info(info)
		, view(view)
		, assoc_views(assoc_views) {
		_c_ctx.system_id = system_id;
		_c_ctx.impl = this;
	}

	/**
	 * Pointer for ecsact C system execution
	 */
	inline cptr_t cptr() noexcept {
		return reinterpret_cast<cptr_t>(&_c_ctx);
	}

	/**
	 * Pointer for ecsact C system execution
	 */
	inline const_cptr_t cptr() const noexcept {
		return reinterpret_cast<const_cptr_t>(&_c_ctx);
	}

	template<typename C>
	void add(const C& component) {
		using ecsact::entt::component_added;
		using ecsact::entt::component_removed;
		using ecsact::entt::detail::pending_add;
		using namespace std::string_literals;

#ifndef NDEBUG
		{
			const bool already_has_component =
				info.registry.template all_of<pending_add<C>>(entity);
			if(already_has_component) {
				std::string err_msg = "Cannot call ctx.add() multiple times. ";
				err_msg += "Added component: "s + typeid(C).name();
				throw std::runtime_error(err_msg.c_str());
			}
		}
#endif

		if constexpr(std::is_empty_v<C>) {
			info.registry.template emplace<pending_add<C>>(entity);
		} else {
			info.registry.template emplace<pending_add<C>>(entity, component);
		}

		if constexpr(!C::transient) {
			if(info.registry.template all_of<component_removed<C>>(entity)) {
				info.registry.template remove<component_removed<C>>(entity);
			} else {
				info.registry.template emplace<component_added<C>>(entity);
			}
		}
	}

	// void add(ecsact_component_like_id component_id, const void* component_data)
	// { 	using ecsact::entt::detail::mp_for_each_available_component;
	//
	// 	mp_for_each_available_component<package>([&]<typename C>(const C&) {
	// 		if(ecsact_id_cast<ecsact_component_like_id>(C::id) == component_id) {
	// 			add<C>(*static_cast<const C*>(component_data));
	// 		}
	// 	});
	// }

	template<typename C>
	void remove() {
		using ecsact::entt::component_added;
		using ecsact::entt::component_removed;
		using ecsact::entt::detail::pending_remove;
		using ecsact::entt::detail::temp_storage;

#ifndef NDEBUG
		[[maybe_unused]] auto component_name = typeid(C).name();

		{
			const bool already_has_component =
				info.registry.template all_of<C>(entity);
			if(!already_has_component) {
				std::string err_msg = "Cannot call ctx.remove() multiple times. ";
				err_msg += "Removed component: ";
				err_msg += component_name;
				throw std::runtime_error(err_msg.c_str());
			}
		}
#endif

		if constexpr(!C::transient) {
			if(info.registry.template all_of<component_added<C>>(entity)) {
				info.registry.template remove<component_added<C>>(entity);
				info.registry.template remove<component_removed<C>>(entity);
			} else {
				info.registry.template emplace_or_replace<component_removed<C>>(entity);
			}
			if constexpr(!std::is_empty_v<C>) {
				auto& temp = info.registry.template storage<temp_storage<C>>();

				// Store current value of component for the before_remove event later
				if(temp.contains(entity)) {
					temp.get(entity).value = info.registry.template get<C>(entity);
				} else {
					temp.emplace(entity, info.registry.template get<C>(entity));
				}
			}
		}

		// TODO(zaucy): Look into if emplace_or_replace is necessary instead of
		//              just replace.
		info.registry.template emplace_or_replace<pending_remove<C>>(entity);
	}

	// void remove(ecsact_component_like_id component_id) {
	// 	using ecsact::entt::detail::mp_for_each_available_component;
	//
	// 	mp_for_each_available_component<package>([&]<typename C>(const C&) {
	// 		if(ecsact_id_cast<ecsact_component_like_id>(C::id) == component_id) {
	// 			remove<C>();
	// 		}
	// 	});
	// }

	template<typename C>
		requires(!std::is_empty_v<C>)
	const C& get() {
#ifndef NDEBUG
		assert(
			info.registry.template all_of<C>(entity) &&
			"context get called for wrong component type. "
			"Check system capabilities."
		);
#endif

		return view.template get<C>(entity);
	}

	// 	void get(ecsact_component_like_id component_id, void* out_component_data)
	// { 		using boost::mp11::mp_assign; 		using boost::mp11::mp_flatten;
	// using boost::mp11::mp_for_each; 		using boost::mp11::mp_push_back; using
	// boost::mp11::mp_unique; 		using
	// ecsact::entt_mp11_util::mp_map_find_value_or;
	//
	// 		using readonly_components = typename caps_info::readonly_components;
	// 		using readwrite_components = typename caps_info::readwrite_components;
	// 		using gettable_components = mp_assign<
	// 			::ecsact::mp_list<>,
	// 			mp_unique<mp_flatten<
	// 				mp_push_back<readonly_components, readwrite_components>,
	// 				::ecsact::mp_list<>>>>;
	//
	// #ifndef NDEBUG
	// 		[[maybe_unused]] bool        found_gettable_component = false;
	// 		[[maybe_unused]] const char* get_component_name = "";
	// 		[[maybe_unused]] auto        gettable_components_type_name =
	// 			typeid(gettable_components).name();
	// #endif // NDEBUG
	//
	// 		mp_for_each<gettable_components>([&]<typename C>(C) {
	// 			if(ecsact_id_cast<ecsact_component_like_id>(C::id) == component_id) {
	// 				if constexpr(!std::is_empty_v<C>) {
	// 					C& out_component = *reinterpret_cast<C*>(out_component_data);
	// 					out_component = get<C>();
	// #ifndef NDEBUG
	// 					get_component_name = typeid(C).name();
	// 					found_gettable_component = true;
	// #endif // NDEBUG
	// 				}
	// 			}
	// 		});
	//
	// #ifndef NDEBUG
	// 		assert(found_gettable_component);
	// #endif // NDEBUG
	// 	}

	template<typename C>
		requires(!std::is_empty_v<C>)
	void update(const C& c) {
		using boost::mp11::mp_any;
		using boost::mp11::mp_apply;
		using boost::mp11::mp_bind_front;
		using boost::mp11::mp_list;
		using boost::mp11::mp_transform_q;
		using ecsact::entt_mp11_util::mp_map_find_value_or;

		using ecsact::entt::component_changed;
		using ecsact::entt::detail::beforechange_storage;

		constexpr bool is_writable = mp_apply<
			mp_any,
			mp_transform_q<
				mp_bind_front<std::is_same, std::remove_cvref_t<C>>,
				typename caps_info::readwrite_components>>::value;

		static_assert(is_writable);

		C&    comp = view.template get<C>(entity);
		auto& beforechange = view.template get<beforechange_storage<C>>(entity);
		if(!beforechange.set) {
			beforechange.value = comp;
			beforechange.set = true;

			info.registry.template emplace_or_replace<component_changed<C>>(entity);
		}
		comp = c;
	}

	// void update(
	// 	ecsact_component_like_id component_id,
	// 	const void*              component_data
	// ) {
	// 	using boost::mp11::mp_for_each;
	// 	using boost::mp11::mp_list;
	// 	using boost::mp11::mp_map_find;
	// 	using ecsact::entt_mp11_util::mp_map_find_value_or;
	//
	// 	using readwrite_components = typename caps_info::readwrite_components;
	//
	// 	mp_for_each<readwrite_components>([&]<typename C>(C) {
	// 		if(ecsact_id_cast<ecsact_component_like_id>(C::id) == component_id) {
	// 			update<C>(*reinterpret_cast<const C*>(component_data));
	// 		}
	// 	});
	// }

	template<typename ComponentT>
	bool has() {
		return info.registry.template all_of<ComponentT>(entity);
	}

	bool has(ecsact_component_like_id component_id) {
		using boost::mp11::mp_for_each;
		using boost::mp11::mp_list;
		using boost::mp11::mp_map_find;
		using ecsact::entt_mp11_util::mp_map_find_value_or;

		using optional_components = typename caps_info::optional_components;

		bool result = false;
		mp_for_each<optional_components>([&]<typename C>(C) {
			if(ecsact_cast_id<ecsact_component_like_id>(C::id) == component_id) {
				result = has<C>();
			}
		});
		return result;
	}

	auto generate(
		int                  component_count,
		ecsact_component_id* component_ids,
		const void**         components_data
	) -> void override {
		using ecsact::entt::component_added;
		using ecsact::entt::detail::created_entity;
		using ecsact::entt::detail::mp_for_each_available_component;
		using ecsact::entt::detail::pending_add;

		auto new_entity = info.create_entity();
		info.registry.template emplace<created_entity>(
			new_entity,
			ecsact_generated_entity
		);
		for(int i = 0; component_count > i; ++i) {
			auto component_id = component_ids[i];
			auto component_data = components_data[i];
			mp_for_each_available_component<package>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					if constexpr(std::is_empty_v<C>) {
						info.registry.template emplace<pending_add<C>>(new_entity);
					} else {
						info.registry.template emplace<pending_add<C>>(
							new_entity,
							*static_cast<const C*>(component_data)
						);
					}

					info.registry.template emplace<component_added<C>>(new_entity);
				}
			});
		}
	}

	auto other( //
		ecsact::entt::entity_id other_entity
	) -> ecsact_system_execution_context* override {
		using boost::mp11::mp_first;
		using boost::mp11::mp_for_each;
		using boost::mp11::mp_second;
		using boost::mp11::mp_size;
		using boost::mp11::mp_with_index;

		using associations = typename caps_info::associations;

		ecsact_system_execution_context* return_context = nullptr;

		constexpr auto assoc_count = mp_size<associations>::value;

		std::size_t assoc_index = 0;
		if constexpr(assoc_count > 0) {
			mp_for_each<associations>([&]<typename Assoc>(Assoc) {
				using ComponentT = typename Assoc::component_type;
				constexpr std::size_t offset = Assoc::field_offset;
				const ComponentT& comp = info.registry.template get<ComponentT>(entity);

				ecsact::entt::entity_id field_entity_value =
					*reinterpret_cast<const ecsact_entity_id*>(
						reinterpret_cast<const char*>(&comp) + offset
					);

				if(field_entity_value == other_entity) {
					using boost::mp11::mp_size;
					mp_with_index<mp_size<associations>::value>(assoc_index, [&](auto I) {
						using other_context_t = std::tuple_element_t<I, others_t>;
						auto& other_context = std::get<I>(others);
						auto& assoc_view = std::get<I>(assoc_views);

						static_assert(
							std::tuple_size_v<
								typename other_context_t::value_type::association_views_type> ==
								0,
							"Recursive associations not supported yet."
						);

						if(!other_context) {
							static std::tuple<> empty_views;
							other_context.emplace(
								info,
								_c_ctx.system_id,
								assoc_view,
								empty_views, // see static assertion above
								field_entity_value,
								parent,
								action
							);

							other_context->_c_ctx.association_index =
								static_cast<unsigned>(assoc_index + 1);
						}

						return_context = &other_context->_c_ctx;
					});

					assoc_index += 1;
				}
			});
		}

		return return_context;
	}
};

} // namespace ecsact_entt_rt
