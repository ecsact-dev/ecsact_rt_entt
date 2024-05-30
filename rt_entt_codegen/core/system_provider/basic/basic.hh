#pragma once

#include <string>
#include <unordered_map>

#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"

namespace ecsact::rt_entt_codegen::core::provider {

class basic : system_provider {
	using system_provider::system_provider;

public:
	auto initialization(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void final;

	auto before_make_view_or_group(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
															options,
		std::vector<std::string>& additional_view_components
	) -> void = 0;

	auto after_make_view_or_group(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;

	auto context_function_action(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;
	auto context_function_add(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;
	auto context_function_remove(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;
	auto context_function_get(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;
	auto context_function_update(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;
	auto context_function_has(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;
	auto context_function_generate(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;
	auto context_function_parent(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;

	auto context_function_other(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto entity_iteration(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;
	auto pre_exec_system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;

	auto system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto post_exec_system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;
	auto post_iteration(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;

private:
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>
		sys_caps;

	std::string view_type_name;
};
} // namespace ecsact::rt_entt_codegen::core::provider
