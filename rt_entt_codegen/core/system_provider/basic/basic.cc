#include "basic.hh"

#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/core/system_provider/system_ctx_functions.hh"

using ecsact::rt_entt_codegen::provider::context_action_impl;
using ecsact::rt_entt_codegen::provider::context_add_impl;
using ecsact::rt_entt_codegen::provider::context_generate_impl;
using ecsact::rt_entt_codegen::provider::context_get_impl;
using ecsact::rt_entt_codegen::provider::context_has_impl;
using ecsact::rt_entt_codegen::provider::context_parent_impl;
using ecsact::rt_entt_codegen::provider::context_remove_impl;
using ecsact::rt_entt_codegen::provider::context_update_impl;

auto ecsact::rt_entt_codegen::core::provider::basic::initialization(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> void {
	sys_caps =
		ecsact::meta::system_capabilities(sys_like_id_variant.get_sys_like_id());

	view_type_name = "view";
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_header(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> void {
	if(names.action_var_name) {
		ctx.write("const void* action_data = nullptr;\n");
	}
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_action(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	context_action_impl(ctx, sys_like_id_variant);
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_add(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	auto adds_comps = std::vector<ecsact_component_like_id>{};
	context_add_impl(ctx, sys_like_id_variant);
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_remove(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	context_remove_impl(ctx, sys_like_id_variant, system_details, view_type_name);
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_get(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	context_get_impl(ctx, sys_like_id_variant, system_details, view_type_name);
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_update(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	context_update_impl(ctx, sys_like_id_variant, system_details, view_type_name);
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_has(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	context_has_impl(ctx, sys_like_id_variant, system_details);
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_generate(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	context_generate_impl(ctx, sys_like_id_variant, system_details);
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_parent(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	context_parent_impl(ctx, sys_like_id_variant);
	return HANDLED;
}
