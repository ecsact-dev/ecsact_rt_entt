#pragma once

#include <string>
#include <optional>
#include <format>
#include <variant>

#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/parallel.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/runtime/meta.hh"

namespace ecsact::rt_entt_codegen::core {

using system_like_id_variant_t =
	std::variant<ecsact_system_id, ecsact_action_id>;

struct print_execute_systems_options {
	system_like_id_variant_t sys_like_id;
	std::string              system_name;
	std::string              registry_var_name;
	std::string              parent_context_var_name;
	/// only set if system is an action
	std::optional<std::string> action_var_name;

	auto as_system() const -> ecsact_system_id {
		return std::get<ecsact_system_id>(sys_like_id);
	}

	auto as_action() const -> ecsact_action_id {
		return std::get<ecsact_action_id>(sys_like_id);
	}

	auto is_system() const -> bool {
		return std::holds_alternative<ecsact_system_id>(sys_like_id);
	}

	auto is_action() const -> bool {
		return std::holds_alternative<ecsact_action_id>(sys_like_id);
	}

	auto get_sys_like_id() const -> ecsact_system_like_id {
		return std::visit(
			[](auto&& arg) { return static_cast<ecsact_system_like_id>(arg); },
			sys_like_id
		);
	}
};

auto print_child_systems(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details&        details,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& sys_details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options options
) -> void;
} // namespace ecsact::rt_entt_codegen::core
