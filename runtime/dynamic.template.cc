#include <ecsact/runtime/dynamic.h>

#include "common.template.hh"
#include "system_execution_context.hh"

using namespace ecsact_entt_rt;

using system_execution_context_t =
	decltype(ecsact_entt_rt::runtime)::system_execution_context;

static system_execution_context_t& cast_ctx
	( ecsact_system_execution_context*  context
	)
{
	return *reinterpret_cast<system_execution_context_t*>(context);
}

static const system_execution_context_t& cast_ctx
	( const ecsact_system_execution_context*  context
	)
{
	return *reinterpret_cast<const system_execution_context_t*>(context);
}

const void* ecsact_system_execution_context_action
	( ecsact_system_execution_context*  context
	)
{
	return cast_ctx(context).action;
}

void ecsact_system_execution_context_add
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	, const void*                       component_data
	)
{
	cast_ctx(context).add(
		static_cast<::ecsact::component_id>(component_id),
		component_data
	);
}

void ecsact_system_execution_context_remove
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	cast_ctx(context).remove(static_cast<::ecsact::component_id>(component_id));
}

void* ecsact_system_execution_context_get
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	return cast_ctx(context).get(static_cast<::ecsact::component_id>(component_id));
}

bool ecsact_system_execution_context_has
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	return cast_ctx(context).has(static_cast<::ecsact::component_id>(component_id));
}

const ecsact_system_execution_context* ecsact_system_execution_context_parent
	( ecsact_system_execution_context*  context
	)
{
	return cast_ctx(context).parent;
}

bool ecsact_system_execution_context_same
	( const ecsact_system_execution_context* a
	, const ecsact_system_execution_context* b
	)
{
	return cast_ctx(a).entity == cast_ctx(b).entity;
}

void ecsact_system_execution_context_generate
	( ecsact_system_execution_context*  context
	, int                               component_count
	, ecsact_component_id*              component_ids
	, const void**                      components_data
	)
{
	cast_ctx(context).generate(
		component_count,
		reinterpret_cast<::ecsact::component_id*>(component_ids),
		components_data
	);
}
