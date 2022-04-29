#include <ecsact/runtime/dynamic.h>

#include <boost/mp11.hpp>

#include "common.template.hh"
#include "system_execution_context.hh"

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
	using boost::mp11::mp_identity;
	using boost::mp11::mp_transform;
	using boost::mp11::mp_flatten;
	using boost::mp11::mp_push_back;

	using all_systems = mp_flatten<mp_push_back<
		typename package::actions,
		typename package::systems
	>>;
	using all_systems_identities = mp_transform<mp_identity, all_systems>;

	mp_for_each<all_systems_identities>([&]<typename S>(mp_identity<S>) {
		// S is a system or an action so we must cast the potential action id to a
		// system id.
		if(static_cast<::ecsact::system_id>(S::id) == ctx->system_id) {
			fn(*static_cast<system_execution_context<package, S>*>(ctx->impl));
		}
	});
}

const void* ecsact_system_execution_context_action
	( ecsact_system_execution_context*  context
	)
{
	return context->impl->action;
}

void ecsact_system_execution_context_add
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	, const void*                       component_data
	)
{
	cast_and_use_ctx(context, [&](auto& context) {
		context.add(
			static_cast<::ecsact::component_id>(component_id),
			component_data
		);
	});
}

void ecsact_system_execution_context_remove
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	cast_and_use_ctx(context, [&](auto& context) {
		context.remove(static_cast<::ecsact::component_id>(component_id));
	});
}

void* ecsact_system_execution_context_get
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	void* component = nullptr;

	cast_and_use_ctx(context, [&](auto& context) {
		component = context.get(static_cast<::ecsact::component_id>(component_id));
	});

	return component;
}

bool ecsact_system_execution_context_has
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	bool has_component = false;

	cast_and_use_ctx(context, [&](auto& context) {
		has_component = context.has(
			static_cast<::ecsact::component_id>(component_id)
		);
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
			reinterpret_cast<::ecsact::component_id*>(component_ids),
			components_data
		);
	});
}
