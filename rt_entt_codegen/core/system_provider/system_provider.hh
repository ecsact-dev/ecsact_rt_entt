#pragma once

#include <vector>
#include <string>
#include <variant>

#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/shared/system_variant.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen::core::provider {
enum handle_exclusive_provide {
	HANDLED = 0,
	NOT_HANDLED = 1,
};

class system_provider {
public:
	system_provider(system_like_id_variant sys_like_id_variant);
	virtual ~system_provider();

	virtual auto initialization(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void = 0;

	virtual auto before_make_view_or_group(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names,
		std::vector<std::string>& additional_view_components
	) -> void = 0;

	virtual auto after_make_view_or_group(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void = 0;

	virtual auto context_function_header(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void = 0;
	virtual auto context_function_action(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_add(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_remove(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_get(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_update(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_has(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_generate(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_parent(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_other(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;

	virtual auto pre_entity_iteration(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void = 0;
	virtual auto entity_iteration(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void = 0;
	virtual auto pre_exec_system_impl(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void = 0;
	virtual auto system_impl(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide = 0;
	virtual auto post_exec_system_impl(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void = 0;
	virtual auto post_iteration(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void = 0;

protected:
	system_like_id_variant     sys_like_id_variant;
	ecsact_entt_system_details system_details;
};
} // namespace ecsact::rt_entt_codegen::core::provider
