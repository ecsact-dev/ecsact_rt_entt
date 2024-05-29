#pragma once

#include <vector>
#include <string>

#include "ecsact/codegen/plugin.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class system_provider {
	virtual auto initialization() -> void = 0;

	virtual auto before_make_view_or_group(
		std::vector<std::string>& additional_view_components
	) -> void = 0;

	virtual auto after_make_view_or_gorup() -> void = 0;

	virtual auto context_function_add() -> void = 0;
	virtual auto context_function_remove() -> void = 0;
	virtual auto context_function_get() -> void = 0;
	virtual auto context_function_update() -> void = 0;
	virtual auto context_function_has() -> void = 0;

	virtual auto entity_iteration() -> void = 0;
	virtual auto pre_exec_system_impl() -> void = 0;
	virtual auto system_impl() -> void = 0;
	virtual auto post_exec_system_impl() -> void = 0;
	virtual auto post_iteration() -> void = 0;
};
} // namespace ecsact::rt_entt_codegen::core::provider
