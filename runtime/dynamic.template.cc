#include <boost/mp11.hpp>
#include "ecsact/runtime/dynamic.h"
#include "ecsact/entt/detail/system_execution_context.hh"

#include "common.template.hh"

using namespace ecsact_entt_rt;

namespace {
	using package = typename decltype(ecsact_entt_rt::runtime)::package;
}

template<typename Fn>
static void cast_and_use_ctx
	( ecsact_system_execution_context*  ctx
	, Fn&&                              fn
	)
{
	using boost::mp11::mp_for_each;
	using boost::mp11::mp_transform;
	using boost::mp11::mp_flatten;
	using boost::mp11::mp_push_back;

	using all_systems = mp_flatten<mp_push_back<
		typename package::actions,
		typename package::systems
	>>;

	mp_for_each<all_systems>([&]<typename S>(S) {
		if(ecsact_id_cast<ecsact_system_like_id>(S::id) == ctx->system_id) {
			fn(*static_cast<system_execution_context<package, S>*>(ctx->impl));
		}
	});
}

void ecsact_system_execution_context_action
	( ecsact_system_execution_context*  context
	, void*                             out_action_data
	)
{
	using boost::mp11::mp_for_each;

	auto action_id = static_cast<ecsact_action_id>(context->system_id);

	cast_and_use_ctx(context, [&](auto& context) {
		mp_for_each<typename package::actions>([&]<typename A>(A) {
			if(A::id == action_id) {
				A& out_action = *reinterpret_cast<A*>(out_action_data);
				out_action = *reinterpret_cast<const A*>(context.action);
			}
		});
	});
}

void ecsact_system_execution_context_add
	( ecsact_system_execution_context*  context
	, ecsact_component_like_id          component_id
	, const void*                       component_data
	)
{
	cast_and_use_ctx(context, [&](auto& context) {
		context.add(component_id, component_data);
	});
}

void ecsact_system_execution_context_remove
	( ecsact_system_execution_context*  context
	, ecsact_component_like_id          component_id
	)
{
	cast_and_use_ctx(context, [&](auto& context) {
		context.remove(component_id);
	});
}

void ecsact_system_execution_context_get
	( ecsact_system_execution_context*  context
	, ecsact_component_like_id          component_id
	, void*                             out_component_data
	)
{
	cast_and_use_ctx(context, [&](auto& context) {
		context.get(component_id, out_component_data);
	});
}

void ecsact_system_execution_context_update
	( ecsact_system_execution_context*  context
	, ecsact_component_like_id          component_id
	, const void*                       component_data
	)
{
	cast_and_use_ctx(context, [&](auto& context) {
		context.update(component_id, component_data);
	});
}

bool ecsact_system_execution_context_has
	( ecsact_system_execution_context*  context
	, ecsact_component_like_id          component_id
	)
{
	bool has_component = false;

	cast_and_use_ctx(context, [&](auto& context) {
		has_component = context.has(component_id);
	});

	return has_component;
}

const ecsact_system_execution_context* ecsact_system_execution_context_parent
	( ecsact_system_execution_context*  context
	)
{
	return context->impl->parent;
}

bool ecsact_system_execution_context_same
	( const ecsact_system_execution_context* a
	, const ecsact_system_execution_context* b
	)
{
	return a->impl->entity == b->impl->entity;
}

void ecsact_system_execution_context_generate
	( ecsact_system_execution_context*  context
	, int                               component_count
	, ecsact_component_id*              component_ids
	, const void**                      components_data
	)
{
	cast_and_use_ctx(context, [&](auto& context) {
		context.generate(
			component_count,
			component_ids,
			components_data
		);
	});
}

#ifdef ECSACT_ENTT_RUNTIME_DYNAMIC_SYSTEM_IMPLS
bool ecsact_set_system_execution_impl
	( ecsact_system_like_id         system_id
	, ecsact_system_execution_impl  system_exec_impl
	)
{
	return runtime.set_system_execution_impl(system_id, system_exec_impl);
}
#endif

ecsact_system_like_id ecsact_system_execution_context_id
	( ecsact_system_execution_context* context
	)
{
	return context->system_id;
}

ecsact_system_execution_context* ecsact_system_execution_context_other
	( ecsact_system_execution_context*  context
	, ecsact_entity_id                  entity_id
	)
{
	ecsact_system_execution_context* other = nullptr;
	cast_and_use_ctx(context, [&](auto& context) {
		other = context.other(entity_id);
	});
	return other;
}
