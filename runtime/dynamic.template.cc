#include <boost/mp11.hpp>
#include "ecsact/runtime/dynamic.h"
#include "ecsact/entt/detail/system_execution_context.hh"
#include "ecsact/entt/detail/meta_util.hh"

#include "common.template.hh"

using namespace ecsact_entt_rt;

namespace {
using package = typename decltype(ecsact_entt_rt::runtime)::package;
}

template<typename Fn>
static void cast_and_use_ctx(ecsact_system_execution_context* ctx, Fn&& fn) {
	using ecsact::entt::detail::mp_for_each_available_system_like;

	mp_for_each_available_system_like<package>([&]<typename S>(S) {
		if(ecsact_id_cast<ecsact_system_like_id>(S::id) == ctx->system_id) {
			using boost::mp11::mp_size;
			using caps_info = ecsact::system_capabilities_info<S>;
			using associations = typename caps_info::associations;

			if(ctx->association_index == -1) {
				using context_type = system_execution_context<package, caps_info>;

				fn(*static_cast<context_type*>(ctx->impl));
			} else {
				using boost::mp11::mp_with_index;

				if constexpr(mp_size<associations>::value > 0) {
					mp_with_index<mp_size<associations>::value>(
						ctx->association_index,
						[&](auto I) {
							using boost::mp11::mp_at;
							using boost::mp11::mp_size_t;
							using assoc = mp_at<associations, mp_size_t<I>>;
							using context_type = system_execution_context<package, assoc>;
							fn(*static_cast<context_type*>(ctx->impl));
						}
					);
				}
			}
		}
	});
}

void ecsact_system_execution_context_action(
	ecsact_system_execution_context* context,
	void*                            out_action_data
) {
	using ecsact::entt::detail::mp_for_each_available_action;

	auto action_id = static_cast<ecsact_action_id>(context->system_id);

	cast_and_use_ctx(context, [&](auto& context) {
		mp_for_each_available_action<package>([&]<typename A>(A) {
			if(A::id == action_id) {
				A& out_action = *reinterpret_cast<A*>(out_action_data);
				out_action = *reinterpret_cast<const A*>(context.action);
			}
		});
	});
}

void ecsact_system_execution_context_add(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         component_id,
	const void*                      component_data
) {
	cast_and_use_ctx(context, [&](auto& context) {
		context.add(component_id, component_data);
	});
}

void ecsact_system_execution_context_remove(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         component_id
) {
	cast_and_use_ctx(context, [&](auto& context) {
		context.remove(component_id);
	});
}

static const auto _get_fns = []() {
	using ecsact::entt::detail::mp_for_each_available_system_like;

	// TODO(zaucy): Figure out how to get the actual max amount of systems in a
	// package
	constexpr auto result_size = 100;
	auto           result =
		std::array<decltype(&ecsact_system_execution_context_get), result_size>{};

	mp_for_each_available_system_like<package>([&]<typename S>(S) {
		using boost::mp11::mp_size;
		using caps_info = ecsact::system_capabilities_info<S>;
		using associations = typename caps_info::associations;

		result[static_cast<size_t>(S::id)] = //
			[](
				ecsact_system_execution_context* context,
				ecsact_component_like_id         component_id,
				void*                            out_component_data
			) {
				if(context->association_index == -1) {
					using context_type = system_execution_context<package, caps_info>;
					static_cast<context_type*>(context->impl)
						->get(component_id, out_component_data);
				} else {
					using boost::mp11::mp_with_index;

					if constexpr(mp_size<associations>::value > 0) {
						mp_with_index<mp_size<associations>::value>(
							context->association_index,
							[&](auto I) {
								using boost::mp11::mp_at;
								using boost::mp11::mp_size_t;
								using assoc = mp_at<associations, mp_size_t<I>>;
								using context_type = system_execution_context<package, assoc>;
								static_cast<context_type*>(context->impl)
									->get(component_id, out_component_data);
							}
						);
					}
				}
			};
	});

	return result;
}();

void ecsact_system_execution_context_get(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         component_id,
	void*                            out_component_data
) {
	_get_fns[static_cast<size_t>(context->system_id)](
		context,
		component_id,
		out_component_data
	);
}

void ecsact_system_execution_context_update(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         component_id,
	const void*                      component_data
) {
	cast_and_use_ctx(context, [&](auto& context) {
		context.update(component_id, component_data);
	});
}

bool ecsact_system_execution_context_has(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         component_id
) {
	bool has_component = false;

	cast_and_use_ctx(context, [&](auto& context) {
		has_component = context.has(component_id);
	});

	return has_component;
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
	ecsact_entity_id entity = {};
	cast_and_use_ctx(
		const_cast<ecsact_system_execution_context*>(context),
		[&](auto& context) { entity = context.get_ecsact_entity_id(); }
	);
	return entity;
}

void ecsact_system_execution_context_generate(
	ecsact_system_execution_context* context,
	int                              component_count,
	ecsact_component_id*             component_ids,
	const void**                     components_data
) {
	cast_and_use_ctx(context, [&](auto& context) {
		context.generate(component_count, component_ids, components_data);
	});
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
	ecsact_system_execution_context* other = nullptr;
	cast_and_use_ctx(context, [&](auto& context) {
		other = context.other(entity_id);
	});
	return other;
}
