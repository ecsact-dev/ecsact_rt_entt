#pragma once

#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class parallel final : public system_provider {
public:
	using system_provider::system_provider;

	auto entity_iteration(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::function<void()>                             iter_func
	) -> handle_exclusive_provide;
};
} // namespace ecsact::rt_entt_codegen::core::provider
