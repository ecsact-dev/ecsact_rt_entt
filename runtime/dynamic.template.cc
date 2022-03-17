#include <ecsact/runtime/dynamic.h>

#include "common.template.hh"

using namespace ecsact_entt_rt;

const void* ecsact_system_execution_context_action
	( ecsact_system_execution_context*  context
	)
{
	return context->action;
}

void ecsact_system_execution_context_add
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	, const void*                       component_data
	)
{
	context->add(
		static_cast<::ecsact::component_id>(component_id),
		component_data
	);
}

void ecsact_system_execution_context_remove
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	context->remove(static_cast<::ecsact::component_id>(component_id));
}

void* ecsact_system_execution_context_get
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	return context->get(static_cast<::ecsact::component_id>(component_id));
}

bool ecsact_system_execution_context_has
	( ecsact_system_execution_context*  context
	, ecsact_component_id               component_id
	)
{
	return context->has(static_cast<::ecsact::component_id>(component_id));
}

const ecsact_system_execution_context* ecsact_system_execution_context_parent
	( ecsact_system_execution_context*  context
	)
{
	return context->parent;
}

bool ecsact_system_execution_context_same
	( const ecsact_system_execution_context* a
	, const ecsact_system_execution_context* b
	)
{
	return a->entity == b->entity;
}

void ecsact_system_execution_context_generate
	( ecsact_system_execution_context*  context
	, int                               component_count
	, ecsact_component_id*              component_ids
	, const void**                      components_data
	)
{
	context->generate(
		component_count,
		reinterpret_cast<::ecsact::component_id*>(component_ids),
		components_data
	);
}
