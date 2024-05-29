#include "association.hh"

#include <map>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/meta.hh"

ecsact::rt_entt_codegen::core::provider::association::association() {
}

static auto print_other_contexts(
	ecsact::codegen_plugin_context&                                    ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&         details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options options
) -> std::map<ecsact::rt_entt_codegen::other_key, std::string> {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::component_name;
	using ecsact::meta::decl_full_name;
	using ecsact::meta::get_child_system_ids;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;
	using ecsact::rt_entt_codegen::other_key;
	using ecsact::rt_entt_codegen::system_util::create_context_struct_name;
	using ecsact::rt_entt_codegen::system_util::create_context_var_name;
	using ecsact::rt_entt_codegen::system_util::get_unique_view_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	std::map<other_key, std::string> other_views;

	for(auto& assoc_detail : details.association_details) {
		auto struct_name = create_context_struct_name(assoc_detail.component_id);
		auto struct_header = struct_name + " : ecsact_system_execution_context ";

		auto view_name = get_unique_view_name();
		other_views.insert(
			other_views.end(),
			std::pair(
				other_key{
					.component_like_id = assoc_detail.component_id,
					.field_id = assoc_detail.field_id //
				},
				view_name
			)
		);

		auto other_details =
			ecsact_entt_system_details::from_capabilities(assoc_detail.capabilities);

		ecsact::rt_entt_codegen::util::make_view(
			ctx,
			view_name,
			"registry",
			other_details
		);

		ctx.write(std::format("using {}_t = decltype({});\n", view_name, view_name)
		);

		block(ctx, "struct " + struct_header, [&] {
			using namespace std::string_literals;
			using ecsact::rt_entt_codegen::util::decl_cpp_ident;
			using std::views::transform;

			ctx.write(std::format("{}_t* view;\n", view_name));
			ctx.write("\n");
			print_sys_exec_ctx_action(ctx, other_details, options);
			print_sys_exec_ctx_add(ctx, other_details, assoc_detail.capabilities);
			print_sys_exec_ctx_remove(
				ctx,
				other_details,
				assoc_detail.capabilities,
				view_name
			);
			print_sys_exec_ctx_get(ctx, other_details, view_name);
			print_sys_exec_ctx_update(ctx, other_details, view_name);
			print_sys_exec_ctx_has(ctx, other_details);
			print_sys_exec_ctx_generate(ctx, other_details);
			print_sys_exec_ctx_parent(ctx);
			print_sys_exec_ctx_other(ctx, other_details);
		});
		ctx.write(";\n\n");

		auto type_name = cpp_identifier(decl_full_name(assoc_detail.component_id));
		auto context_name = create_context_var_name(assoc_detail.component_id);

		ctx.write(struct_name, " ", context_name, ";\n\n");

		ctx.write(context_name, ".view = &", view_name, ";\n");
		ctx.write(context_name, ".parent_ctx = nullptr;\n\n");
		ctx.write(context_name, ".registry = &", options.registry_var_name, ";\n");
	}

	return other_views;
}
