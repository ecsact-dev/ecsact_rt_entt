#pragma once

#include <variant>

#include "ecsact/runtime/common.h"

namespace ecsact::rt_entt_codegen {
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
			[](auto&& arg) { return ecsact_id_cast<ecsact_system_like_id>(arg); },
			*this
		);
	}
};
} // namespace ecsact::rt_entt_codegen
