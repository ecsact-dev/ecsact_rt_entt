#pragma once

#include <string>
#include <map>

#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class association : system_provider {
	using system_provider::system_provider;

public:
	auto initialization(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void {
	}

	auto before_make_view_or_group(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
															options,
		std::vector<std::string>& additional_view_components
	) -> void {
	}

	auto after_make_view_or_group(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void final;

	auto context_function_header(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void final;

	auto context_function_action(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_add(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_remove(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_get(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_update(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_has(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_generate(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_parent(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_other(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;

	auto pre_entity_iteration(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void final;

	auto entity_iteration(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void {
	}

	auto pre_exec_system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void final;
	auto system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide final;
	auto post_exec_system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void {};
	auto post_iteration(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void {};

private:
	std::map<other_key, std::string> other_view_names;

	std::map<ecsact_component_like_id, std::string> components_with_entity_fields;

	auto print_other_contexts(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void;
};
} // namespace ecsact::rt_entt_codegen::core::provider
