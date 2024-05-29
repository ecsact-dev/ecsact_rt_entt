#pragma once

#include <cstdint>
#include <string>
#include <map>

#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/core/system_provider/system_provider.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class association : system_provider {
public:
	association();

	auto initialization() -> void = 0;

	auto before_make_view_or_group(
		std::vector<std::string>& additional_view_components
	) -> void = 0;

	auto after_make_view_or_gorup() -> void = 0;

	auto context_function_add() -> void = 0;
	auto context_function_remove() -> void = 0;
	auto context_function_get() -> void = 0;
	auto context_function_update() -> void = 0;
	auto context_function_has() -> void = 0;

	auto entity_iteration() -> void = 0;
	auto pre_exec_system_impl() -> void = 0;
	auto system_impl() -> void = 0;
	auto post_exec_system_impl() -> void = 0;
	auto post_iteration() -> void = 0;

private:
	std::map<other_key, std::string> other_views;
};
} // namespace ecsact::rt_entt_codegen::core::provider
