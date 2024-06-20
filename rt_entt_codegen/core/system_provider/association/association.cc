#include "association.hh"

#include <map>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using ecsact::cc_lang_support::cpp_identifier;
using ecsact::cpp_codegen_plugin_util::block;
using ecsact::meta::decl_full_name;
using ecsact::rt_entt_codegen::ecsact_entt_system_details;
using ecsact::rt_entt_codegen::system_util::create_context_struct_name;
using ecsact::rt_entt_codegen::system_util::create_context_var_name;
using ecsact::rt_entt_codegen::system_util::get_unique_view_name;
using ecsact::rt_entt_codegen::util::method_printer;

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

using namespace ecsact::rt_entt_codegen::core;

auto provider::association::initialization(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> void {
	auto assoc_ids = ecsact::meta::system_assoc_ids(sys_like_id);
	for(auto assoc_id : assoc_ids) {
		auto assoc_comp_id =
			ecsact::meta::system_assoc_component_id(sys_like_id, assoc_id);
		auto assoc_field_ids =
			ecsact::meta::system_assoc_fields(sys_like_id, assoc_id);
		assoc_view_names.insert({assoc_id, get_unique_view_name()});

		for(auto field_id : assoc_field_ids) {
			auto field_type = ecsact::meta::get_field_type(assoc_comp_id, field_id);
			if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN &&
				 field_type.type.builtin == ECSACT_ENTITY_TYPE) {
				auto compo_id = ecsact_id_cast<ecsact_composite_id>(assoc_comp_id);
				assoc_fields[compo_id].push_back(field_id);
				assoc_composites[assoc_id].insert(compo_id);
			} else if(field_type.kind == ECSACT_TYPE_KIND_FIELD_INDEX) {
				auto compo_id = field_type.type.field_index.composite_id;
				assoc_fields[compo_id].push_back(field_id);
				assoc_composites[assoc_id].insert(compo_id);
			} else {
				// Should never get here. Association fields may only be an indexed
				// field or entity field.
				assert(false);
			}
		}
	}
}

auto provider::association::context_function_header(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> void {
	ctx.write(
		"std::unordered_map<ecsact_entity_id,ecsact_system_execution_"
		"context*> "
		"other_contexts;\n\n"
	);
}

auto provider::association::after_make_view_or_group(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> void {
	auto assoc_ids = ecsact::meta::system_assoc_ids(sys_like_id);
	for(auto assoc_id : assoc_ids) {
		auto assoc_caps =
			ecsact::meta::system_assoc_capabilities(sys_like_id, assoc_id);
		auto assoc_system_details =
			ecsact_entt_system_details::from_capabilities(assoc_caps);
		auto make_view_opts = util::make_view_options(assoc_system_details);
		make_view_opts.view_var_name = assoc_view_names.at(assoc_id);
		make_view_opts.registry_var_name = names.registry_var_name;

		util::make_view(ctx, make_view_opts);
	}
}

auto provider::association::context_function_other(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> handle_exclusive_provide {
	context_other_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

auto provider::association::entity_iteration(
	codegen_plugin_context& ctx,
	const common_vars&      names,
	std::function<void()>   iter_func
) -> handle_exclusive_provide {
	block(ctx, "for(auto entity : view)", [&] {
		for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
			auto assoc_caps =
				ecsact::meta::system_assoc_capabilities(sys_like_id, assoc_id);
			auto assoc_system_details =
				ecsact_entt_system_details::from_capabilities(assoc_caps);
			auto make_view_opts = util::make_view_options(assoc_system_details);
			make_view_opts.view_var_name = assoc_view_names.at(assoc_id);
			make_view_opts.registry_var_name = names.registry_var_name;

			for(auto compo_id : assoc_composites.at(assoc_id)) {
			}

			util::make_view(ctx, make_view_opts);
		}

		for(auto&& [assoc_id, compo_ids] : assoc_composites) {
			for(auto compo_id : compo_ids) {
				auto field_ids = assoc_fields.at(compo_id);
				auto compo_cpp_ident = cpp_identifier(decl_full_name(compo_id));

				ctx.write(std::format(
					"{0}.storage({3}.storage<{1}>(static_cast<::entt::id_type>(::ecsact::"
					"entt::detail::"
					"hash_vals({1}::"
					"id, {2}))));\n",
					assoc_view_names.at(assoc_id),
					compo_cpp_ident,
					util::comma_delim(
						field_ids |
						std::views::transform([&](auto field_id) -> std::string {
							return std::format(
								"view.get<{}>(entity).{}",
								compo_cpp_ident,
								ecsact::meta::field_name(compo_id, field_id)
							);
						})
					),
					names.registry_var_name
				));
			}
		}

		for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
			auto assoc_comp_id =
				ecsact::meta::system_assoc_component_id(sys_like_id, assoc_id);
			auto assoc_field_ids =
				ecsact::meta::system_assoc_fields(sys_like_id, assoc_id);
			auto assoc_comp_cpp_ident = cpp_identifier(decl_full_name(assoc_comp_id));
		}

		for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
			ctx.write(std::format(
				"auto {0}_itr = {0}.begin();\n",
				assoc_view_names.at(assoc_id)
			));
		}

		block(ctx, "for(;;)", [&] {
			for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
				ctx.write(std::format(
					"if({0}_itr == {0}.end()) break;\n",
					assoc_view_names.at(assoc_id)
				));
			}

			iter_func();

			for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
				ctx.write(std::format("{}_itr++;\n", assoc_view_names.at(assoc_id)));
			}
		});
	});

	return HANDLED;
}

auto provider::association::pre_entity_iteration(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> void {
	print_other_contexts(ctx, names);
}

auto provider::association::pre_exec_system_impl(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> void {
}

auto provider::association::system_impl(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> handle_exclusive_provide {
	ctx.write("system_impl(&context);\n");
	return HANDLED;
}

auto provider::association::print_other_contexts(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> void {
	auto assoc_ids = ecsact::meta::system_assoc_ids(sys_like_id);
	for(auto assoc_id : assoc_ids) {
		auto assoc_comp_id =
			ecsact::meta::system_assoc_component_id(sys_like_id, assoc_id);
		auto struct_name = create_context_struct_name(assoc_comp_id);
		auto context_name = create_context_var_name(assoc_comp_id);
		auto struct_header = struct_name + " : ecsact_system_execution_context ";
		auto assoc_caps =
			ecsact::meta::system_assoc_capabilities(sys_like_id, assoc_id);
		auto assoc_system_details =
			ecsact_entt_system_details::from_capabilities(assoc_caps);

		ctx.write(std::format(
			"using {0}_t = decltype({0});\n",
			assoc_view_names.at(assoc_id)
		));

		block(ctx, "struct " + struct_header, [&] {
			using namespace std::string_literals;
			using ecsact::rt_entt_codegen::util::decl_cpp_ident;
			using std::views::transform;

			ctx.write(std::format("{}_t* view;\n", assoc_view_names.at(assoc_id)));
			ctx.write("\n");
			print_other_ctx_action(ctx);
			print_other_ctx_add(
				ctx,
				std::unordered_map{assoc_caps.begin(), assoc_caps.end()},
				assoc_system_details
			);
			print_other_ctx_remove(
				ctx,
				std::unordered_map{assoc_caps.begin(), assoc_caps.end()},
				assoc_system_details,
				assoc_view_names.at(assoc_id)
			);
			print_other_ctx_get(
				ctx,
				assoc_system_details,
				assoc_view_names.at(assoc_id)
			);
			print_other_ctx_update(
				ctx,
				assoc_system_details,
				assoc_view_names.at(assoc_id)
			);
			print_other_ctx_has(ctx, assoc_system_details);
			print_other_ctx_generate(ctx, assoc_system_details);
			print_other_ctx_parent(ctx, sys_like_id);
			print_other_ctx_other(ctx, assoc_system_details);
		});
		ctx.write(";\n\n");

		ctx.write(struct_name, " ", context_name, ";\n\n");

		ctx.write(context_name, ".view = &", assoc_view_names.at(assoc_id), ";\n");
		ctx.write(context_name, ".parent_ctx = nullptr;\n\n");
		ctx.write(context_name, ".registry = &", names.registry_var_name, ";\n");
	}
}

auto provider::association::print_other_ctx_action( //
	codegen_plugin_context& ctx
) -> void {
	auto printer = //
		method_printer{ctx, "action"}
			.parameter("void*", "out_action_data")
			.return_type("void final");

	context_action_impl(ctx, sys_like_id);
}

auto provider::association::print_other_ctx_add(
	codegen_plugin_context&           ctx,
	const capability_t&               other_caps,
	const ecsact_entt_system_details& details
) -> void {
	auto printer = //
		method_printer{ctx, "add"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("const void*", "component_data")
			.return_type("void final");

	context_add_impl(ctx, other_caps);
}

auto provider::association::print_other_ctx_remove(
	codegen_plugin_context&           ctx,
	const capability_t&               other_caps,
	const ecsact_entt_system_details& details,
	const std::string&                view_type_name
) -> void {
	auto printer = //
		method_printer{ctx, "remove"}
			.parameter("ecsact_component_like_id", "component_id")
			.return_type("void final");

	context_remove_impl(ctx, other_caps, details, view_type_name);
}

auto provider::association::print_other_ctx_get(
	codegen_plugin_context&           ctx,
	const ecsact_entt_system_details& details,
	const std::string&                view_type_name
) -> void {
	auto printer = //
		method_printer{ctx, "get"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("void*", "out_component_data")
			.return_type("void final");

	context_get_impl(ctx, sys_like_id, details, view_type_name);
}

auto provider::association::print_other_ctx_update(
	codegen_plugin_context&           ctx,
	const ecsact_entt_system_details& details,
	const std::string&                view_type_name
) -> void {
	auto printer = //
		method_printer{ctx, "update"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("const void*", "component_data")
			.return_type("void final");

	context_update_impl(ctx, sys_like_id, details, view_type_name);
}

auto provider::association::print_other_ctx_has(
	codegen_plugin_context&           ctx,
	const ecsact_entt_system_details& details
) -> void {
	auto printer = //
		method_printer{ctx, "has"}
			.parameter("ecsact_component_like_id", "component_id")
			.return_type("bool final");

	context_has_impl(ctx, sys_like_id, details);
}

auto provider::association::print_other_ctx_generate(
	codegen_plugin_context&           ctx,
	const ecsact_entt_system_details& details
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
	codegen_plugin_context&       ctx,
	const system_like_id_variant& sys_like_id
) -> void {
	auto printer = //
		method_printer{ctx, "parent"} //
			.return_type("const ecsact_system_execution_context* final");

	context_parent_impl(ctx, sys_like_id);
}

auto provider::association::print_other_ctx_other(
	codegen_plugin_context&           ctx,
	const ecsact_entt_system_details& details
) -> void {
	auto printer = //
		method_printer{ctx, "other"}
			.parameter("ecsact_entity_id", "entity")
			.return_type("ecsact_system_execution_context* final");

	context_other_impl(ctx, sys_like_id, details);
}
