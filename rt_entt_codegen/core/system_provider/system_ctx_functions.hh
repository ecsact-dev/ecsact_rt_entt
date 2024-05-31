#pragma once

#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "rt_entt_codegen/shared/system_variant.hh"

namespace ecsact::rt_entt_codegen::provider {
auto context_action_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant
) -> void;
auto context_add_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant
) -> void;
auto context_remove_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void;
auto context_get_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void;
auto context_update_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void;
auto context_has_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;
auto context_generate_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;
auto context_parent_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant
) -> void;
auto context_other_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id_variant,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;
} // namespace ecsact::rt_entt_codegen::provider
