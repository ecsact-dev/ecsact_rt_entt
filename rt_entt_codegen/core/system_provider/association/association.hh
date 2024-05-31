#pragma once

#include <string>
#include <map>

#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/core/system_provider/system_ctx_functions.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class association : public system_provider {
public:
	using system_provider::system_provider;

	auto initialization(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void {
	}

	auto before_make_view_or_group(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names,
		std::vector<std::string>& additional_view_components
	) -> void {
	}

	auto after_make_view_or_group(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void final;

	auto context_function_header(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void final;

	auto context_function_action(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_add(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_remove(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_get(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_update(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_has(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_generate(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_parent(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide {
		return NOT_HANDLED;
	}

	auto context_function_other(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide final;

	auto pre_entity_iteration(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void final;

	auto entity_iteration(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void {
	}

	auto pre_exec_system_impl(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void final;
	auto system_impl(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> handle_exclusive_provide final;
	auto post_exec_system_impl(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void {};
	auto post_iteration(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void {};

private:
	std::map<other_key, std::string> other_view_names;

	std::map<ecsact_component_like_id, std::string> components_with_entity_fields;

	auto print_other_contexts(
		ecsact::codegen_plugin_context&                                       ctx,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
	) -> void;

	auto print_other_ctx_action(ecsact::codegen_plugin_context& ctx) -> void;
	auto print_other_ctx_add(
		ecsact::codegen_plugin_context&                            ctx,
		const capability_t&                                        other_caps,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
	) -> void;

	auto print_other_ctx_remove(
		ecsact::codegen_plugin_context&                            ctx,
		const capability_t&                                        other_caps,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const std::string&                                         view_type_name
	) -> void;

	auto print_other_ctx_get(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const std::string&                                         view_type_name
	) -> void;

	auto print_other_ctx_update(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const std::string&                                         view_type_name
	) -> void;

	auto print_other_ctx_has(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
	) -> void;

	auto print_other_ctx_generate(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
	) -> void;

	auto print_other_ctx_parent(
		ecsact::codegen_plugin_context& ctx,
		const system_like_id_variant&   sys_like_id_variant
	) -> void;

	auto print_other_ctx_other(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
	) -> void;
};
} // namespace ecsact::rt_entt_codegen::core::provider
