#include "basic.hh"

#include "ecsact/runtime/meta.hh"
#include "rt_entt_codegen/core/system_provider/system_ctx_functions.hh"

using namespace ecsact::rt_entt_codegen::core;

auto provider::basic::initialization(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	sys_caps = ecsact::meta::system_capabilities(sys_like_id.get_sys_like_id());

	view_type_name = "view";
}

auto provider::basic::context_function_header(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	if(names.action_var_name) {
		ctx.write("const void* action_data = nullptr;\n");
	}
}

auto provider::basic::context_function_action(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_action_impl(ctx, sys_like_id);
	return HANDLED;
}

auto provider::basic::context_function_add(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	auto sys_caps =
		ecsact::meta::system_capabilities(sys_like_id.get_sys_like_id());

	context_add_impl(ctx, sys_caps);
	return HANDLED;
}

auto provider::basic::context_function_remove(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	auto sys_caps =
		ecsact::meta::system_capabilities(sys_like_id.get_sys_like_id());

	context_remove_impl(ctx, sys_caps, system_details, view_type_name);
	return HANDLED;
}

auto provider::basic::context_function_get(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_get_impl(ctx, sys_like_id, system_details, view_type_name);
	return HANDLED;
}

auto provider::basic::context_function_update(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_update_impl(ctx, sys_like_id, system_details, view_type_name);
	return HANDLED;
}

auto provider::basic::context_function_has(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_has_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

auto provider::basic::context_function_generate(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_generate_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

auto provider::basic::context_function_parent(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_parent_impl(ctx, sys_like_id);
	return HANDLED;
}

auto provider::basic::context_function_other(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	ctx.write("return nullptr;");
	return HANDLED;
}

auto provider::basic::system_impl(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	ctx.write("system_impl(&context);\n");
	return HANDLED;
}
