#pragma once

#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class notify : system_provider {
public:
	notify(
		ecsact::codegen_plugin_context&                                     ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details&          details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_options& options
	);

	auto initialization() -> void {
	}

	auto before_make_view_or_group(
		std::vector<std::string>& additional_view_components
	) -> void final;

	auto after_make_view_or_group() -> void {
	}

	auto context_function_add() -> void {
	}

	auto context_function_remove() -> void {
	}

	auto context_function_get() -> void {
	}

	auto context_function_update() -> void {
	}

	auto context_function_has() -> void {
	}

	auto after_system_context() -> void {
	}

	auto entity_iteration() -> void {
	}

	auto pre_exec_system_impl() -> void {
	}

	auto system_impl() -> void {
	}

	auto post_exec_system_impl() -> void {
	}

	auto post_iteration() -> void {
	}

private:
	ecsact::codegen_plugin_context&                                     ctx;
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&          details;
	const ecsact::rt_entt_codegen::core::print_execute_systems_options& options;

	/*
	 * Prints the specialized views for ecsact_system_notify_settings components
	 */
	auto print_system_notify_views(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
		ecsact_system_like_id                                      system_id,
		std::string                                                registry_name
	) -> void;
};
} // namespace ecsact::rt_entt_codegen::core::provider
