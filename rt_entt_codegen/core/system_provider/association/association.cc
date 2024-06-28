#include "association.hh"

#include <map>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

using namespace ecsact::rt_entt_codegen::core;

auto provider::association::context_function_header(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> void {
	ctx.write(
		"std::unordered_map<ecsact_entity_id,ecsact_system_execution_"
		"context*> "
		"other_contexts;\n\n"
	);
}

auto provider::association::after_make_view_or_group(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> void {
}

auto provider::association::context_function_other(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> handle_exclusive_provide {
	context_other_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

auto provider::association::pre_entity_iteration(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> void {
	print_other_contexts(ctx, names);
}

auto provider::association::pre_exec_system_impl(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
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

	ctx.write("auto found_assoc_entities = 0;\n");

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

auto provider::association::system_impl(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> handle_exclusive_provide {
	using ecsact::cpp_codegen_plugin_util::block;

	// we need to check if we found any invalid associations
	block(
		ctx,
		"if(found_assoc_entities == " + std::to_string(other_view_names.size()) +
			")",
		[&] {
			auto child_ids = ecsact::meta::get_child_system_ids(sys_like_id);
			if(child_ids.empty()) {
				ctx.write("system_impl(&context);\n");
			}
		}
	);

	return HANDLED;
}

auto provider::association::print_other_contexts(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
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

	for(auto& assoc_detail : system_details.association_details) {
		auto struct_name = create_context_struct_name(assoc_detail.component_id);
		auto context_name = create_context_var_name(assoc_detail.component_id);

		auto struct_header = struct_name + " : ecsact_system_execution_context ";

		auto view_type_name = get_unique_view_name();
		other_view_names.insert(
			other_view_names.end(),
			std::pair(
				other_key{
					.component_like_id = assoc_detail.component_id,
					.field_id = assoc_detail.field_id //
				},
				view_type_name
			)
		);

		auto other_details =
			ecsact_entt_system_details::from_capabilities(assoc_detail.capabilities);

		ecsact::rt_entt_codegen::util::make_view(
			ctx,
			view_type_name,
			"registry",
			other_details
		);

		ctx.write(std::format(
			"using {}_t = decltype({});\n",
			view_type_name,
			view_type_name
		));

		block(ctx, "struct " + struct_header, [&] {
			using namespace std::string_literals;
			using ecsact::rt_entt_codegen::util::decl_cpp_ident;
			using std::views::transform;

			ctx.write(std::format("{}_t* view;\n", view_type_name));
			ctx.write("\n");
			print_other_ctx_action(ctx);
			print_other_ctx_add(ctx, assoc_detail.capabilities, other_details);
			print_other_ctx_remove(
				ctx,
				assoc_detail.capabilities,
				other_details,
				view_type_name
			);
			print_other_ctx_get(ctx, other_details, view_type_name);
			print_other_ctx_update(ctx, other_details, view_type_name);
			print_other_ctx_has(ctx, other_details);
			print_other_ctx_generate(ctx, other_details);
			print_other_ctx_parent(ctx, sys_like_id);
			print_other_ctx_other(ctx, other_details);
		});
		ctx.write(";\n\n");

		ctx.write(struct_name, " ", context_name, ";\n\n");

		ctx.write(context_name, ".view = &", view_type_name, ";\n");
		ctx.write(context_name, ".parent_ctx = nullptr;\n\n");
		ctx.write(context_name, ".registry = &", names.registry_var_name, ";\n");
	}
}

using ecsact::rt_entt_codegen::util::method_printer;

auto provider::association::print_other_ctx_action(
	ecsact::codegen_plugin_context& ctx
) -> void {
	auto printer = //
		method_printer{ctx, "action"}
			.parameter("void*", "out_action_data")
			.return_type("void final");

	context_action_impl(ctx, sys_like_id);
}

auto provider::association::print_other_ctx_add(
	ecsact::codegen_plugin_context&                            ctx,
	const capability_t&                                        other_caps,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	auto printer = //
		method_printer{ctx, "add"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("const void*", "component_data")
			.return_type("void final");

	context_add_impl(ctx, other_caps);
}

auto provider::association::print_other_ctx_remove(
	ecsact::codegen_plugin_context&                            ctx,
	const capability_t&                                        other_caps,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
	auto printer = //
		method_printer{ctx, "remove"}
			.parameter("ecsact_component_like_id", "component_id")
			.return_type("void final");

	context_remove_impl(ctx, other_caps, details, view_type_name);
}

auto provider::association::print_other_ctx_get(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
	auto printer = //
		method_printer{ctx, "get"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("void*", "out_component_data")
			.return_type("void final");

	context_get_impl(ctx, sys_like_id, details, view_type_name);
}

auto provider::association::print_other_ctx_update(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
	auto printer = //
		method_printer{ctx, "update"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("const void*", "component_data")
			.return_type("void final");

	context_update_impl(ctx, sys_like_id, details, view_type_name);
}

auto provider::association::print_other_ctx_has(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	auto printer = //
		method_printer{ctx, "has"}
			.parameter("ecsact_component_like_id", "component_id")
			.return_type("bool final");

	context_has_impl(ctx, sys_like_id, details);
}

auto provider::association::print_other_ctx_generate(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	auto printer = //
		method_printer{ctx, "generate"}
			.parameter("int", "component_count")
			.parameter("ecsact_component_id*", "component_ids")
			.parameter("const void**", "components_data")
			.return_type("void final");

	context_generate_impl(ctx, sys_like_id, details);
}

auto provider::association::print_other_ctx_parent(
	ecsact::codegen_plugin_context&                        ctx,
	const ecsact::rt_entt_codegen::system_like_id_variant& sys_like_id
) -> void {
	auto printer = //
		method_printer{ctx, "parent"} //
			.return_type("const ecsact_system_execution_context* final");

	context_parent_impl(ctx, sys_like_id);
}

auto provider::association::print_other_ctx_other(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	auto printer = //
		method_printer{ctx, "other"}
			.parameter("ecsact_entity_id", "entity")
			.return_type("ecsact_system_execution_context* final");

	context_other_impl(ctx, sys_like_id, details);
}
