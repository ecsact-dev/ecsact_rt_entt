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

auto init_lazy(
	ecsact::codegen_plugin_context&     ctx,
	const print_execute_systems_options options,
	std::vector<std::string>&           additional_view_components
) -> void;

auto sort_print_access_lazy(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& sys_details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options options
) -> void;

auto print_system_execution_context(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details&        details,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& sys_details,
	const print_execute_systems_options                        options
) -> void;

// using ecsact::cpp_codegen_plugin_util::block;

// auto sys_caps = ecsact::meta::system_capabilities(options.sys_like_id);

// block(ctx, "struct : ecsact_system_execution_context ", [&] {
// 	ctx.write("view_t* view;\n");

// 	ctx.write(
// 		"std::unordered_map<ecsact_entity_id,ecsact_system_execution_"
// 		"context*> "
// 		"other_contexts;\n\n"
// 	);

// 	if(options.action_var_name) {
// 		ctx.write("const void* action_data = nullptr;\n");
// 	}

// 	ctx.write("\n");
// 	print_sys_exec_ctx_action(ctx, sys_details, options.sys_like_id);
// 	print_sys_exec_ctx_add(ctx, sys_details, sys_caps);
// 	print_sys_exec_ctx_remove(ctx, sys_details, sys_caps, "view");
// 	print_sys_exec_ctx_get(ctx, sys_details, "view");
// 	print_sys_exec_ctx_update(ctx, sys_details, "view");
// 	print_sys_exec_ctx_has(ctx, sys_details);
// 	print_sys_exec_ctx_generate(ctx, sys_details);
// 	print_sys_exec_ctx_parent(ctx);
// 	print_sys_exec_ctx_other(ctx, sys_details);
// });
// ctx.write("context;\n\n");

// ctx.write("context.registry = &", options.registry_var_name, ";\n");
// if(options.action_var_name) {
// 	ctx.write("context.action_data = ", *options.action_var_name, ";\n\n");
// }

// ctx.write(
// 	"context.id = ecsact_id_cast<ecsact_system_like_id>(::",
// 	options.system_name,
// 	"::id);\n"
// );
// ctx.write("context.parent_ctx = ", options.parent_context_var_name, ";\n");
// ctx.write("context.view = &view;\n\n");

auto print_child_systems(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details&        details,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& sys_details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options options
) -> void;
} // namespace ecsact::rt_entt_codegen::core
