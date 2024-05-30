#pragma once

#include <vector>
#include <string>
#include <variant>

#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"

namespace ecsact::rt_entt_codegen::core::provider {
enum handle_exclusive_provide {
	HANDLED = 0,
	NOT_HANDLED = 1,
};

struct system_like_id_variant
	: std::variant<ecsact_system_id, ecsact_action_id> {
	using variant::variant;

	auto as_system() const -> ecsact_system_id {
		return std::get<ecsact_system_id>(*this);
	}

	auto as_action() const -> ecsact_action_id {
		return std::get<ecsact_action_id>(*this);
	}

	auto is_system() const -> bool {
		return std::holds_alternative<ecsact_system_id>(*this);
	}

	auto is_action() const -> bool {
		return std::holds_alternative<ecsact_action_id>(*this);
	}

	auto get_sys_like_id() const -> ecsact_system_like_id {
		return std::visit(
			[](auto&& arg) { return static_cast<ecsact_system_like_id>(arg); },
			*this
		);
	}
};

class system_provider {
public:
	system_provider(system_like_id_variant sys_like_id_variant);

protected:
	system_like_id_variant sys_like_id_variant;

private:
	virtual auto initialization(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;

	virtual auto before_make_view_or_group(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
															options,
		std::vector<std::string>& additional_view_components
	) -> void = 0;

	virtual auto after_make_view_or_group(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;

	virtual auto context_function_header(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;
	virtual auto context_function_action(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_add(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_remove(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_get(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_update(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_has(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_generate(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_parent(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto context_function_other(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;

	virtual auto pre_entity_iteration(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;
	virtual auto entity_iteration(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;
	virtual auto pre_exec_system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;
	virtual auto system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> handle_exclusive_provide = 0;
	virtual auto post_exec_system_impl(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;
	virtual auto post_iteration(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_var_names&
			options
	) -> void = 0;
};
} // namespace ecsact::rt_entt_codegen::core::provider
