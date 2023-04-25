#include <unordered_map>
#include <boost/mp11.hpp>
#include "ecsact/runtime/dynamic.h"
#include "ecsact/entt/detail/system_execution_context.hh"
#include "ecsact/entt/detail/meta_util.hh"

#include "common.template.hh"

using namespace ecsact_entt_rt;

namespace {
using package = typename decltype(ecsact_entt_rt::runtime)::package;
}

template<typename SystemLikesList>
using max_associations = std::integral_constant<size_t, 64>; // TODO

/**
 * Array of arrays containing function pointer for each system in list
 *
 * @example
 *    ```
 *    auto fns = context_fns_t<FnType, SystemsList>{};
 *
 *    auto fn = fns[MySystem::id][0];
 *    fn(...);
 *    ```
 */
template<typename IdType, typename FuncType, typename SystemLikesList>
using context_fns_t = std::unordered_map<
	IdType,
	std::array<FuncType, max_associations<SystemLikesList>::value + 1>>;

template<typename Action, typename ContextType>
static inline void _action_fn(
	ecsact_system_execution_context* context,
	void*                            out_action_data
) {
	auto&   typed_context = *static_cast<ContextType*>(context->impl);
	Action& out_action = *reinterpret_cast<Action*>(out_action_data);
	out_action = *reinterpret_cast<const Action*>(typed_context.action);
}

static auto _action_fns = [] {
	using boost::mp11::mp_for_each;
	using boost::mp11::mp_size;
	using actions = ecsact::entt::detail::mp_all_actions_t<package>;

	auto result = context_fns_t<
		ecsact_action_id,
		decltype(&ecsact_system_execution_context_action),
		actions>{};

	result.reserve(mp_size<actions>::value);

	mp_for_each<actions>([&]<typename A>(A) {
		using boost::mp11::mp_size;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_for_each;

		using caps_info = ecsact::system_capabilities_info<A>;
		using associations = typename caps_info::associations;

		result[A::id][0] =
			&_action_fn<A, system_execution_context<package, caps_info>>;
		if constexpr(mp_size<associations>::value > 0) {
			mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
				using boost::mp11::mp_at;
				using boost::mp11::mp_size_t;
				using assoc = mp_at<associations, mp_size_t<I>>;
				result[A::id][I + 1] =
					&_action_fn<A, system_execution_context<package, assoc>>;
			});
		}
	});

	return std::as_const(result);
}();

void ecsact_system_execution_context_action(
	ecsact_system_execution_context* context,
	void*                            out_action_data
) {
	auto action_id = static_cast<ecsact_action_id>(context->system_id);

	assert(_action_fns.contains(action_id));
	assert(_action_fns[action_id].size() > context->association_index);
	assert(_action_fns[action_id][context->association_index] != nullptr);

	_action_fns[action_id][context->association_index](context, out_action_data);
}

template<typename Component, typename ContextType>
static inline void _add_fn(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         component_id,
	const void*                      component_data
) {
	auto& typed_context = *static_cast<ContextType*>(context->impl);
	assert(
		ecsact_id_cast<ecsact_component_like_id>(Component::id) == component_id
	);

	if constexpr(std::is_empty_v<Component>) {
		typed_context.template add<Component>();
	} else {
		typed_context.template add<Component>(
			*static_cast<const Component*>(component_data)
		);
	}
}

static auto _add_fns = [] {
	using boost::mp11::mp_for_each;
	using boost::mp11::mp_size;
	using boost::mp11::mp_append;
	using system_likes = mp_append<
		ecsact::entt::detail::mp_all_actions_t<package>,
		ecsact::entt::detail::mp_all_systems_t<package>>;

	auto result = std::unordered_map<
		ecsact_system_like_id,
		std::array<
			std::unordered_map<
				ecsact_component_like_id,
				decltype(&ecsact_system_execution_context_add)>,
			max_associations<system_likes>::value>>{};

	result.reserve(mp_size<system_likes>::value);

	mp_for_each<system_likes>([&]<typename S>(S) {
		using boost::mp11::mp_size;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_for_each;

		using caps_info = ecsact::system_capabilities_info<S>;
		using associations = typename caps_info::associations;
		using context_type = system_execution_context<package, caps_info>;

		mp_for_each<typename context_type::adds_components>([&]<typename C>(C) {
			result[ecsact_id_cast<ecsact_system_like_id>(S::id)][0]
						[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
							&_add_fn<C, context_type>;
		});

		if constexpr(mp_size<associations>::value > 0) {
			mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
				using boost::mp11::mp_at;
				using boost::mp11::mp_size_t;
				using assoc = mp_at<associations, mp_size_t<I>>;
				using context_type = system_execution_context<package, assoc>;
				mp_for_each<typename context_type::adds_components>([&]<typename C>(C) {
					result[ecsact_id_cast<ecsact_system_like_id>(S::id)][I + 1]
								[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
									&_add_fn<C, context_type>;
				});
			});
		}
	});

	return std::as_const(result);
}();

void ecsact_system_execution_context_add(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	const void*                      component_data
) {
	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(context->system_id);

	assert(_add_fns.contains(sys_like_id));
	auto& _sys_add_fns = _add_fns[sys_like_id];

	assert(_sys_add_fns.size() > context->association_index);
	auto& _sys_ctx_add_fns = _sys_add_fns[context->association_index];

	assert(_sys_ctx_add_fns.contains(comp_id));
	auto& _sys_ctx_add_comp_fn = _sys_ctx_add_fns[comp_id];

	assert(_sys_ctx_add_comp_fn != nullptr);
	_sys_ctx_add_comp_fn(context, comp_id, component_data);
}

template<typename Component, typename ContextType>
static inline void _remove_fn(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         component_id
) {
	auto& typed_context = *static_cast<ContextType*>(context->impl);
	assert(
		ecsact_id_cast<ecsact_component_like_id>(Component::id) == component_id
	);

	typed_context.template remove<Component>();
}

static auto _remove_fns = [] {
	using boost::mp11::mp_for_each;
	using boost::mp11::mp_size;
	using boost::mp11::mp_append;
	using system_likes = mp_append<
		ecsact::entt::detail::mp_all_actions_t<package>,
		ecsact::entt::detail::mp_all_systems_t<package>>;

	auto result = std::unordered_map<
		ecsact_system_like_id,
		std::array<
			std::unordered_map<
				ecsact_component_like_id,
				decltype(&ecsact_system_execution_context_remove)>,
			max_associations<system_likes>::value>>{};

	result.reserve(mp_size<system_likes>::value);

	mp_for_each<system_likes>([&]<typename S>(S) {
		using boost::mp11::mp_size;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_for_each;

		using caps_info = ecsact::system_capabilities_info<S>;
		using associations = typename caps_info::associations;
		using context_type = system_execution_context<package, caps_info>;

		mp_for_each<typename context_type::removes_components>([&]<typename C>(C) {
			result[ecsact_id_cast<ecsact_system_like_id>(S::id)][0]
						[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
							&_remove_fn<C, context_type>;
		});

		if constexpr(mp_size<associations>::value > 0) {
			mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
				using boost::mp11::mp_at;
				using boost::mp11::mp_size_t;
				using assoc = mp_at<associations, mp_size_t<I>>;
				using context_type = system_execution_context<package, assoc>;
				mp_for_each<typename context_type::removes_components>([&]<typename C>(C
																															 ) {
					result[ecsact_id_cast<ecsact_system_like_id>(S::id)][I + 1]
								[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
									&_remove_fn<C, context_type>;
				});
			});
		}
	});

	return std::as_const(result);
}();

void ecsact_system_execution_context_remove(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id
) {
	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(context->system_id);

	assert(_remove_fns.contains(sys_like_id));
	auto& _sys_remove_fns = _remove_fns[sys_like_id];

	assert(_sys_remove_fns.size() > context->association_index);
	auto& _sys_ctx_remove_fns = _sys_remove_fns[context->association_index];

	assert(_sys_ctx_remove_fns.contains(comp_id));
	auto& _sys_ctx_remove_comp_fn = _sys_ctx_remove_fns[comp_id];

	assert(_sys_ctx_remove_comp_fn != nullptr);
	_sys_ctx_remove_comp_fn(context, comp_id);
}

template<typename Component, typename ContextType>
static inline void _get_fn(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         component_id,
	void*                            out_component_data
) {
	auto& typed_context = *static_cast<ContextType*>(context->impl);
	assert(
		ecsact_id_cast<ecsact_component_like_id>(Component::id) == component_id
	);

	Component& out_component = *reinterpret_cast<Component*>(out_component_data);
	out_component = typed_context.template get<Component>();
}

static auto _get_fns = [] {
	using boost::mp11::mp_for_each;
	using boost::mp11::mp_size;
	using boost::mp11::mp_append;
	using system_likes = mp_append<
		ecsact::entt::detail::mp_all_actions_t<package>,
		ecsact::entt::detail::mp_all_systems_t<package>>;

	auto result = std::unordered_map<
		ecsact_system_like_id,
		std::array<
			std::unordered_map<
				ecsact_component_like_id,
				decltype(&ecsact_system_execution_context_get)>,
			max_associations<system_likes>::value>>{};

	result.reserve(mp_size<system_likes>::value);

	mp_for_each<system_likes>([&]<typename S>(S) {
		using boost::mp11::mp_size;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_for_each;

		using caps_info = ecsact::system_capabilities_info<S>;
		using associations = typename caps_info::associations;
		using context_type = system_execution_context<package, caps_info>;

		mp_for_each<typename context_type::gettable_components>([&]<typename C>(C) {
			result[ecsact_id_cast<ecsact_system_like_id>(S::id)][0]
						[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
							&_get_fn<C, context_type>;
		});

		if constexpr(mp_size<associations>::value > 0) {
			mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
				using boost::mp11::mp_at;
				using boost::mp11::mp_size_t;
				using assoc = mp_at<associations, mp_size_t<I>>;
				using context_type = system_execution_context<package, assoc>;
				mp_for_each<typename context_type::gettable_components>(
					[&]<typename C>(C) {
						result[ecsact_id_cast<ecsact_system_like_id>(S::id)][I + 1]
									[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
										&_get_fn<C, context_type>;
					}
				);
			});
		}
	});

	return std::as_const(result);
}();

void ecsact_system_execution_context_get(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	void*                            out_component_data
) {
	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(context->system_id);

	assert(_get_fns.contains(sys_like_id));
	auto& _sys_get_fns = _get_fns[sys_like_id];

	assert(_sys_get_fns.size() > context->association_index);
	auto& _sys_ctx_get_fns = _sys_get_fns[context->association_index];

	assert(_sys_ctx_get_fns.contains(comp_id));
	auto& _sys_ctx_get_comp_fn = _sys_ctx_get_fns[comp_id];

	assert(_sys_ctx_get_comp_fn != nullptr);
	_sys_ctx_get_comp_fn(context, comp_id, out_component_data);
}

template<typename Component, typename ContextType>
static inline void _update_fn(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	const void*                      component_data
) {
	auto& typed_context = *static_cast<ContextType*>(context->impl);
	assert(ecsact_id_cast<ecsact_component_like_id>(Component::id) == comp_id);

	typed_context.template update<Component>(
		*reinterpret_cast<const Component*>(component_data)
	);
}

static auto _update_fns = [] {
	using boost::mp11::mp_for_each;
	using boost::mp11::mp_size;
	using boost::mp11::mp_append;
	using system_likes = mp_append<
		ecsact::entt::detail::mp_all_actions_t<package>,
		ecsact::entt::detail::mp_all_systems_t<package>>;

	auto result = std::unordered_map<
		ecsact_system_like_id,
		std::array<
			std::unordered_map<
				ecsact_component_like_id,
				decltype(&ecsact_system_execution_context_update)>,
			max_associations<system_likes>::value>>{};

	result.reserve(mp_size<system_likes>::value);

	mp_for_each<system_likes>([&]<typename S>(S) {
		using boost::mp11::mp_size;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_for_each;

		using caps_info = ecsact::system_capabilities_info<S>;
		using associations = typename caps_info::associations;
		using context_type = system_execution_context<package, caps_info>;

		mp_for_each<typename context_type::readwrite_components>([&]<typename C>(C
																														 ) {
			result[ecsact_id_cast<ecsact_system_like_id>(S::id)][0]
						[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
							&_update_fn<C, context_type>;
		});

		if constexpr(mp_size<associations>::value > 0) {
			mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
				using boost::mp11::mp_at;
				using boost::mp11::mp_size_t;
				using assoc = mp_at<associations, mp_size_t<I>>;
				using context_type = system_execution_context<package, assoc>;
				mp_for_each<typename context_type::readwrite_components>(
					[&]<typename C>(C) {
						result[ecsact_id_cast<ecsact_system_like_id>(S::id)][I + 1]
									[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
										&_update_fn<C, context_type>;
					}
				);
			});
		}
	});

	return std::as_const(result);
}();

void ecsact_system_execution_context_update(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	const void*                      component_data
) {
	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(context->system_id);

	assert(_update_fns.contains(sys_like_id));
	auto& _sys_update_fns = _update_fns[sys_like_id];

	assert(_sys_update_fns.size() > context->association_index);
	auto& _sys_ctx_update_fns = _sys_update_fns[context->association_index];

	assert(_sys_ctx_update_fns.contains(comp_id));
	auto& _sys_ctx_update_comp_fn = _sys_ctx_update_fns[comp_id];

	assert(_sys_ctx_update_comp_fn != nullptr);
	_sys_ctx_update_comp_fn(context, comp_id, component_data);
}

template<typename Component, typename ContextType>
static inline auto _has_fn(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id
) -> bool {
	auto& typed_context = *static_cast<ContextType*>(context->impl);
	assert(ecsact_id_cast<ecsact_component_like_id>(Component::id) == comp_id);
	return typed_context.template has<Component>();
}

static auto _has_fns = [] {
	using boost::mp11::mp_for_each;
	using boost::mp11::mp_size;
	using boost::mp11::mp_append;
	using system_likes = mp_append<
		ecsact::entt::detail::mp_all_actions_t<package>,
		ecsact::entt::detail::mp_all_systems_t<package>>;

	auto result = std::unordered_map<
		ecsact_system_like_id,
		std::array<
			std::unordered_map<
				ecsact_component_like_id,
				decltype(&ecsact_system_execution_context_has)>,
			max_associations<system_likes>::value>>{};

	result.reserve(mp_size<system_likes>::value);

	mp_for_each<system_likes>([&]<typename S>(S) {
		using boost::mp11::mp_size;
		using boost::mp11::mp_iota_c;
		using boost::mp11::mp_for_each;

		using caps_info = ecsact::system_capabilities_info<S>;
		using associations = typename caps_info::associations;
		using context_type = system_execution_context<package, caps_info>;

		mp_for_each<typename context_type::readwrite_components>([&]<typename C>(C
																														 ) {
			result[ecsact_id_cast<ecsact_system_like_id>(S::id)][0]
						[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
							&_has_fn<C, context_type>;
		});

		if constexpr(mp_size<associations>::value > 0) {
			mp_for_each<mp_iota_c<mp_size<associations>::value>>([&](auto I) {
				using boost::mp11::mp_at;
				using boost::mp11::mp_size_t;
				using assoc = mp_at<associations, mp_size_t<I>>;
				using context_type = system_execution_context<package, assoc>;
				mp_for_each<typename context_type::readwrite_components>(
					[&]<typename C>(C) {
						result[ecsact_id_cast<ecsact_system_like_id>(S::id)][I + 1]
									[ecsact_id_cast<ecsact_component_like_id>(C::id)] =
										&_has_fn<C, context_type>;
					}
				);
			});
		}
	});

	return std::as_const(result);
}();

bool ecsact_system_execution_context_has(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id
) {
	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(context->system_id);

	assert(_has_fns.contains(sys_like_id));
	auto& _sys_has_fns = _has_fns[sys_like_id];

	assert(_sys_has_fns.size() > context->association_index);
	auto& _sys_ctx_has_fns = _sys_has_fns[context->association_index];

	assert(_sys_ctx_has_fns.contains(comp_id));
	auto& _sys_ctx_has_comp_fn = _sys_ctx_has_fns[comp_id];

	assert(_sys_ctx_has_comp_fn != nullptr);
	return _sys_ctx_has_comp_fn(context, comp_id);
}

const ecsact_system_execution_context* ecsact_system_execution_context_parent(
	ecsact_system_execution_context* context
) {
	assert(context->impl->parent && "parent context is NULL");
	return context->impl->parent;
}

bool ecsact_system_execution_context_same(
	const ecsact_system_execution_context* a,
	const ecsact_system_execution_context* b
) {
	return a->impl->entity == b->impl->entity;
}

ecsact_entity_id ecsact_system_execution_context_entity(
	const ecsact_system_execution_context* context
) {
	return context->impl->get_ecsact_entity_id();
}

void ecsact_system_execution_context_generate(
	ecsact_system_execution_context* context,
	int                              component_count,
	ecsact_component_id*             component_ids,
	const void**                     components_data
) {
	context->impl->generate(component_count, component_ids, components_data);
}

#ifdef ECSACT_ENTT_RUNTIME_DYNAMIC_SYSTEM_IMPLS
bool ecsact_set_system_execution_impl(
	ecsact_system_like_id        system_id,
	ecsact_system_execution_impl system_exec_impl
) {
	return runtime.set_system_execution_impl(system_id, system_exec_impl);
}
#endif

ecsact_system_like_id ecsact_system_execution_context_id(
	ecsact_system_execution_context* context
) {
	return context->system_id;
}

ecsact_system_execution_context* ecsact_system_execution_context_other(
	ecsact_system_execution_context* context,
	ecsact_entity_id                 entity_id
) {
	return context->impl->other(entity_id);
}
