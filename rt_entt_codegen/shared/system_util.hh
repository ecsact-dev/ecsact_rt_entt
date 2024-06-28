#pragma once

#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "system_variant.hh"

namespace ecsact::rt_entt_codegen::system_util {

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

/*
 * Checks if a system uses notify and should implement the run_system<S>
 * component in its execution
 */
auto is_notify_system(ecsact_system_like_id system_id) -> bool;

auto is_trivial_system(ecsact_system_like_id system_id) -> bool;

auto get_unique_view_name() -> std::string;

auto get_assoc_context_type_name( //
	system_like_id_variant sys_like_id,
	ecsact_system_assoc_id assoc_id
) -> std::string;

auto get_assoc_context_var_name( //
	system_like_id_variant sys_like_id,
	ecsact_system_assoc_id assoc_id
) -> std::string;

} // namespace ecsact::rt_entt_codegen::system_util
