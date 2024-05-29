#include "association.hh"

#include <map>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/meta.hh"

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

auto print_sys_exec_ctx_action(
	ecsact::codegen_plugin_context&                                    ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&         details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options options
) -> void;

auto print_sys_exec_ctx_add(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	capability_t                                               sys_caps
) -> void;

auto print_sys_exec_ctx_remove(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	capability_t                                               sys_caps,
	const std::string&                                         view_type_name
) -> void;

auto print_sys_exec_ctx_get(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void;

auto print_sys_exec_ctx_update(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void;

auto print_sys_exec_ctx_has(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;

auto print_sys_exec_ctx_generate(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;

auto print_sys_exec_ctx_other(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;

auto print_sys_exec_ctx_parent( //
	ecsact::codegen_plugin_context& ctx
) -> void;

ecsact::rt_entt_codegen::core::provider::association::association(
	ecsact::codegen_plugin_context&                                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&          details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options& options
)
	: ctx(ctx), details(details), options(options) {
}

auto ecsact::rt_entt_codegen::core::provider::association::
	after_make_view_or_group() -> void {
}

auto ecsact::rt_entt_codegen::core::provider::association::after_system_context(
) -> void {
	print_other_contexts();
}

auto ecsact::rt_entt_codegen::core::provider::association::pre_exec_system_impl(
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::system_util::create_context_var_name;

	int comp_iter = 0;

	for(auto [ids, view_name] : other_view_names) {
		if(!components_with_entity_fields.contains(ids.component_like_id)) {
			components_with_entity_fields[ids.component_like_id] =
				"assoc_comp_" + std::to_string(comp_iter++);
		}
	}

	for(auto&& [comp_like_id, comp_var] : components_with_entity_fields) {
		auto comp_name = cpp_identifier(decl_full_name(comp_like_id));
		ctx.write("auto ", comp_var, " = view.get<", comp_name, ">(entity);\n");
	}

	if(!other_view_names.empty()) {
		ctx.write("auto found_assoc_entities = 0;\n");
	}

	for(auto [ids, view_name] : other_view_names) {
		auto field_name = ecsact_meta_field_name(
			ecsact_id_cast<ecsact_composite_id>(ids.component_like_id),
			ids.field_id
		);

		auto entity_field_access =
			components_with_entity_fields.at(ids.component_like_id) + "." +
			field_name;

		auto view_itr_name = view_name + "_itr";
		ctx.write(
			"auto ",
			view_itr_name,
			" = ",
			view_name,
			".find(ecsact::entt::entity_id{",
			entity_field_access,
			"});\n"
		);

		ctx.write(
			"if(",
			view_itr_name,
			" == ",
			view_name,
			".end()) { continue; }\n"
		);

		ctx.write("found_assoc_entities += 1;\n");

		auto context_name = create_context_var_name(ids.component_like_id);
		ctx.write(context_name, ".entity = ", entity_field_access, ";\n");

		ctx.write(
			"context.other_contexts.insert(context.other_contexts.end(), "
			"std::pair(",
			context_name,
			".entity, &",
			context_name,
			"));\n"
		);
	}
}

auto ecsact::rt_entt_codegen::core::provider::association::system_impl()
	-> void {
	using ecsact::cpp_codegen_plugin_util::block;

	// NOTE(Kelwan): It's weird for association to exclusively handle system_impl
	if(other_view_names.empty()) {
		ctx.write("system_impl(&context);\n");
	} else {
		// we need to check if we found any invalid associations
		block(
			ctx,
			"if(found_assoc_entities == " + std::to_string(other_view_names.size()) +
				")",
			[&] { ctx.write("system_impl(&context);\n"); }
		);
	}
}

auto ecsact::rt_entt_codegen::core::provider::association::print_other_contexts(
) -> void {
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

	other_view_names = other_views;
}
