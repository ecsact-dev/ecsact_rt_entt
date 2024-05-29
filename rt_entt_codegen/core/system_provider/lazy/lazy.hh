#pragma once

#include <cstdint>
#include <string>

#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/core/system_provider/system_provider.hh"

namespace ecsact::rt_entt_codegen::core::provider {

class lazy : system_provider {
public:
	// Default init to 0 lazy_it_rate
	lazy(
		ecsact::codegen_plugin_context&                            ctx,
		const ecsact::rt_entt_codegen::ecsact_entt_system_details& sys_details,
		const system_like_id_variant&                              system_like_id_v,
		const std::string&                                         registry_name
	);

	auto initialization() -> void {
	}

	auto before_make_view_or_group(
		std::vector<std::string>& additional_view_components
	) -> void final;

	auto after_make_view_or_gorup() -> void {
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

	auto entity_iteration() -> void {
	}

	auto pre_exec_system_impl() -> void final;

	auto system_impl() -> void {
	}

	auto post_exec_system_impl() -> void final;

	auto post_iteration() -> void {
	}

private:
	ecsact::codegen_plugin_context&                            ctx;
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& system_details;
	const std::string&                                         registry_name;
	const system_like_id_variant& system_like_id_variant;

	std::string system_name;

	std::string exec_start_label_name;
	std::string pending_lazy_exec_struct;
	std::string system_sorting_struct_name;

	int32_t lazy_iteration_rate;
};
} // namespace ecsact::rt_entt_codegen::core::provider
