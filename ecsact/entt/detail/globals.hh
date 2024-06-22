#pragma once

#include <unordered_map>
#include <unordered_set>
#include "entt/entity/registry.hpp"
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/core.h"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/entt/detail/system_execution_context.hh"
#include "ecsact/entt/detail/registry.hh"

/**
 * A small set of globals expected to be available the ecsact_rt_entt_codegen
 * generated source.
 */
namespace ecsact::entt::detail::globals {

/**
 * Ecsact registry ID mapped to EnTT registry instance.
 */
extern std::unordered_map< //
	ecsact_registry_id,
	ecsact::entt::registry_t>
	registries;

/**
 * Stored last registry ID. Used to create new registry IDs.
 */
extern ecsact_registry_id last_registry_id;

/**
 * System-like implementation functions set by dynamic API.
 */
extern std::unordered_map< //
	ecsact_system_like_id,
	ecsact_system_execution_impl>
	system_impls;

/**
 * All components IDs available to rt_entt
 */
extern const std::unordered_set<ecsact_component_id> all_component_ids;

using add_component_fn_sig_t = ecsact_add_error (*)(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data
);

/**
 * ecsact_add_component fn pointers
 *
 * NOTE: This gets is filled in by ecsact_rt_entt_codegen
 */
extern const std::unordered_map< //
	ecsact_component_id,
	add_component_fn_sig_t>
	add_component_fns;

using get_component_fn_sig_t = const void* (*)( //
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	std::uint64_t       assoc_fields_hash
);

/**
 * ecsact_get_component fn pointers
 *
 * NOTE: This gets is filled in by ecsact_rt_entt_codegen
 */
extern const std::unordered_map< //
	ecsact_component_id,
	get_component_fn_sig_t>
	get_component_fns;

using update_component_fn_sig_t = ecsact_update_error (*)(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	std::uint64_t       assoc_fields_hash
);

/**
 * ecsact_update_component fn pointers
 *
 * NOTE: This gets is filled in by ecsact_rt_entt_codegen
 */
extern const std::unordered_map< //
	ecsact_component_id,
	update_component_fn_sig_t>
	update_component_fns;

using remove_component_fn_sig_t = void (*)(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	std::uint64_t       assoc_fields_hash
);

/**
 * ecsact_remove_component fn pointers
 *
 * NOTE: This gets is filled in by ecsact_rt_entt_codegen
 */
extern const std::unordered_map< //
	ecsact_component_id,
	remove_component_fn_sig_t>
	remove_component_fns;

using has_component_fn_sig_t = bool (*)(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	std::uint64_t       assoc_fields_hash
);

/**
 * ecsact_has_component fn pointers
 *
 * NOTE: This gets is filled in by ecsact_rt_entt_codegen
 */
extern const std::unordered_map< //
	ecsact_component_id,
	has_component_fn_sig_t>
	has_component_fns;

/**
 * ecsact_system_execution_context_action fn pointers
 *
 * NOTE: This action is filled in by ecsact_rt_entt_codegen
 */
extern const std::unordered_map<
	ecsact_action_id,
	decltype(&ecsact_system_execution_context_action)>
	exec_ctx_action_fns;

} // namespace ecsact::entt::detail::globals
