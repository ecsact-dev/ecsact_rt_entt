#pragma once

#include <string>
#include <map>

#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/core/system_provider/system_provider.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class association : system_provider {
public:
	association(
		ecsact::codegen_plugin_context&                                     ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details&          details,
		const ecsact::rt_entt_codegen::core::print_execute_systems_options& options
	);

	auto initialization() -> void {
	}

	auto before_make_view_or_group(
		std::vector<std::string>& additional_view_components
	) -> void {
	}

	auto after_make_view_or_group() -> void final;

	auto context_function_add() -> void {};
	auto context_function_remove() -> void {};
	auto context_function_get() -> void {};
	auto context_function_update() -> void {};

	auto context_function_has() -> void {
	}

	auto after_system_context() -> void final;

	auto entity_iteration() -> void {
	}

	auto pre_exec_system_impl() -> void final;
	auto system_impl() -> void final;
	auto post_exec_system_impl() -> void {};
	auto post_iteration() -> void {};

private:
	ecsact::codegen_plugin_context&                                     ctx;
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&          details;
	const ecsact::rt_entt_codegen::core::print_execute_systems_options& options;

	std::map<other_key, std::string> other_view_names;

	std::map<ecsact_component_like_id, std::string> components_with_entity_fields;

	auto print_other_contexts() -> void;
};
} // namespace ecsact::rt_entt_codegen::core::provider
