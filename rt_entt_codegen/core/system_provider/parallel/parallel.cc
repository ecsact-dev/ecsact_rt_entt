#include "parallel.hh"

#include <format>
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using namespace ecsact::rt_entt_codegen::core;

auto provider::parallel::entity_iteration(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::function<void()>                             iter_func
) -> handle_exclusive_provide {
	using ecsact::cpp_codegen_plugin_util::block;

	block(
		ctx,
		"std::for_each(std::execution::par_unseq, view.begin(), "
		"view.end(), [&](auto entity)",
		[&] { iter_func(); }
	);
	ctx.write(");\n");
	return HANDLED;
}

auto provider::parallel::provide_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> handle_exclusive_provide {
	return HANDLED;
}

auto provider::parallel::pre_exec_system_impl_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> void {
	ctx.write(std::format("{} context;\n\n", context_type_name));

	auto system_name =
		cc_lang_support::cpp_identifier(meta::decl_full_name(sys_like_id));

	ctx.write("context.registry = &", names.registry_var_name, ";\n");
	if(names.action_var_name) {
		ctx.write("context.action_data = ", *names.action_var_name, ";\n\n");
	}

	ctx.write(
		"context.id = ecsact_id_cast<ecsact_system_like_id>(::",
		system_name,
		"::id);\n"
	);
	ctx.write("context.parent_ctx = ", names.parent_context_var_name, ";\n");
	ctx.write("context.view = &view;\n\n");
	ctx.write("context.entity = entity;\n");
}
