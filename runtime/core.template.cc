#include <ecsact/runtime/core.h>

#include "common.template.hh"

using namespace ecsact_entt_rt;

ecsact_registry_id ecsact_create_registry
	( const char* registry_name
	)
{
	return static_cast<ecsact_registry_id>(
		runtime.create_registry(registry_name)
	);
}

void ecsact_destroy_registry
	( ecsact_registry_id reg_id
	)
{
	runtime.destroy_registry(static_cast<ecsact::registry_id>(reg_id));
}

void ecsact_clear_registry
	( ecsact_registry_id reg_id
	)
{
	runtime.clear_registry(static_cast<ecsact::registry_id>(reg_id));
}

ecsact_entity_id ecsact_create_entity
	( ecsact_registry_id reg_id
	)
{
	return static_cast<ecsact_entity_id>(
		runtime.create_entity(static_cast<ecsact::registry_id>(reg_id))
	);
}

void ecsact_ensure_entity
	( ecsact_registry_id  reg_id
	, ecsact_entity_id    entity_id
	)
{
	runtime.ensure_entity(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::entity_id>(entity_id)
	);
}

bool ecsact_entity_exists
	( ecsact_registry_id  reg_id
	, ecsact_entity_id    entity_id
	)
{
	return runtime.entity_exists(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::entity_id>(entity_id)
	);
}

void ecsact_destroy_entity
	( ecsact_registry_id  reg_id
	, ecsact_entity_id    entity_id
	)
{
	runtime.destroy_entity(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::entity_id>(entity_id)
	);
}

int ecsact_count_entities
	( ecsact_registry_id  reg_id
	)
{
	return runtime.count_entities(static_cast<ecsact::registry_id>(reg_id));
}

void ecsact_get_entities
	( ecsact_registry_id  reg_id
	, int                 max_entities_count
	, ecsact_entity_id*   out_entities
	, int*                out_entities_count
	)
{
	runtime.get_entities(
		static_cast<ecsact::registry_id>(reg_id),
		max_entities_count,
		reinterpret_cast<ecsact::entity_id*>(out_entities),
		out_entities_count
	);
}

void ecsact_add_component
	( ecsact_registry_id   reg_id
	, ecsact_entity_id     entity_id
	, ecsact_component_id  component_id
	, const void*          component_data
	)
{
	runtime.add_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::entity_id>(entity_id),
		static_cast<ecsact::component_id>(component_id),
		component_data
	);
}

bool ecsact_has_component
	( ecsact_registry_id   reg_id
	, ecsact_entity_id     entity_id
	, ecsact_component_id  component_id
	)
{
	return runtime.has_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::entity_id>(entity_id),
		static_cast<ecsact::component_id>(component_id)
	);
}

const void* ecsact_get_component
	( ecsact_registry_id   reg_id
	, ecsact_entity_id     entity_id
	, ecsact_component_id  component_id
	)
{
	return runtime.get_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::entity_id>(entity_id),
		static_cast<ecsact::component_id>(component_id)
	);
}

void ecsact_update_component
	( ecsact_registry_id   reg_id
	, ecsact_entity_id     entity_id
	, ecsact_component_id  component_id
	, const void*          component_data
	)
{
	runtime.update_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::entity_id>(entity_id),
		static_cast<ecsact::component_id>(component_id),
		component_data
	);
}

void ecsact_remove_component
	( ecsact_registry_id   reg_id
	, ecsact_entity_id     entity_id
	, ecsact_component_id  component_id
	)
{
	runtime.remove_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::entity_id>(entity_id),
		static_cast<ecsact::component_id>(component_id)
	);
}

void ecsact_on_add_component
	( ecsact_registry_id             reg_id
	, ecsact_component_id            component_id
	, ecsact_add_component_callback  callback
	, void*                          callback_user_data
	)
{
	runtime.on_add_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::component_id>(component_id),
		callback,
		callback_user_data
	);
}

void ecsact_off_add_component
	( ecsact_registry_id             reg_id
	, ecsact_component_id            component_id
	, ecsact_add_component_callback  callback
	)
{
	runtime.off_add_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::component_id>(component_id),
		callback
	);
}

void ecsact_on_update_component
	( ecsact_registry_id                reg_id
	, ecsact_component_id               component_id
	, ecsact_update_component_callback  callback
	, void*                             callback_user_data
	)
{
	runtime.on_update_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::component_id>(component_id),
		callback,
		callback_user_data
	);
}

void ecsact_off_update_component
	( ecsact_registry_id                reg_id
	, ecsact_component_id               component_id
	, ecsact_update_component_callback  callback
	)
{
	runtime.off_update_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::component_id>(component_id),
		callback
	);
}

void ecsact_on_before_remove_component
	( ecsact_registry_id                       reg_id
	, ecsact_component_id                      component_id
	, ecsact_before_remove_component_callback  callback
	, void*                                    callback_user_data
	)
{
	runtime.on_before_remove_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::component_id>(component_id),
		callback,
		callback_user_data
	);
}

void ecsact_off_before_remove_component
	( ecsact_registry_id                       reg_id
	, ecsact_component_id                      component_id
	, ecsact_before_remove_component_callback  callback
	)
{
	runtime.off_before_remove_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::component_id>(component_id),
		callback
	);
}

void ecsact_on_after_remove_component
	( ecsact_registry_id                      reg_id
	, ecsact_component_id                     component_id
	, ecsact_after_remove_component_callback  callback
	, void*                                   callback_user_data
	)
{
	runtime.on_after_remove_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::component_id>(component_id),
		callback,
		callback_user_data
	);
}

void ecsact_off_after_remove_component
	( ecsact_registry_id                      reg_id
	, ecsact_component_id                     component_id
	, ecsact_after_remove_component_callback  callback
	)
{
	runtime.off_after_remove_component(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::component_id>(component_id),
		callback
	);
}

void ecsact_on_add_any_component
	( ecsact_registry_id                 reg_id
	, ecsact_add_any_component_callback  callback
	, void*                              callback_user_data
	)
{
	runtime.on_add_any_component(
		static_cast<ecsact::registry_id>(reg_id),
		callback,
		callback_user_data
	);
}

void ecsact_off_add_any_component
	( ecsact_registry_id                 reg_id
	, ecsact_add_any_component_callback  callback
	)
{
	runtime.off_add_any_component(
		static_cast<ecsact::registry_id>(reg_id),
		callback
	);
}

void ecsact_on_update_any_component
	( ecsact_registry_id                    reg_id
	, ecsact_update_any_component_callback  callback
	, void*                                 callback_user_data
	)
{
	runtime.on_update_any_component(
		static_cast<ecsact::registry_id>(reg_id),
		callback,
		callback_user_data
	);	
}

void ecsact_off_update_any_component
	( ecsact_registry_id                    reg_id
	, ecsact_update_any_component_callback  callback
	)
{
	runtime.off_update_any_component(
		static_cast<ecsact::registry_id>(reg_id),
		callback
	);
}

void ecsact_on_before_remove_any_component
	( ecsact_registry_id                           reg_id
	, ecsact_before_remove_any_component_callback  callback
	, void*                                        callback_user_data
	)
{
	runtime.on_before_remove_any_component(
		static_cast<ecsact::registry_id>(reg_id),
		callback,
		callback_user_data
	);	
}

void ecsact_off_before_remove_any_component
	( ecsact_registry_id                           reg_id
	, ecsact_before_remove_any_component_callback  callback
	)
{
	runtime.off_before_remove_any_component(
		static_cast<ecsact::registry_id>(reg_id),
		callback
	);
}

void ecsact_on_after_remove_any_component
	( ecsact_registry_id                          reg_id
	, ecsact_after_remove_any_component_callback  callback
	, void*                                       callback_user_data
	)
{
	runtime.on_after_remove_any_component(
		static_cast<ecsact::registry_id>(reg_id),
		callback,
		callback_user_data
	);
}

void ecsact_off_after_remove_any_component
	( ecsact_registry_id                          reg_id
	, ecsact_after_remove_any_component_callback  callback
	)
{
	runtime.off_after_remove_any_component(
		static_cast<ecsact::registry_id>(reg_id),
		callback
	);
}

void ecsact_push_action
	( ecsact_registry_id  reg_id
	, ecsact_system_id    system_id
	, const void*         action_data
	)
{
	runtime.push_action(
		static_cast<ecsact::registry_id>(reg_id),
		static_cast<ecsact::action_id>(system_id),
		action_data
	);
}

void ecsact_execute_systems
	( ecsact_registry_id  reg_id
	)
{
	runtime.execute_systems(
		static_cast<ecsact::registry_id>(reg_id)
	);
}
