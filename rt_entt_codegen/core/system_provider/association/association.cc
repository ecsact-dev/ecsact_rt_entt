#include "association.hh"

#include <algorithm>
#include <map>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using ecsact::cc_lang_support::cpp_identifier;
using ecsact::cpp_codegen_plugin_util::block;
using ecsact::meta::decl_full_name;
using ecsact::meta::system_assoc_ids;
using ecsact::rt_entt_codegen::ecsact_entt_system_details;
using ecsact::rt_entt_codegen::system_util::get_assoc_context_type_name;
using ecsact::rt_entt_codegen::system_util::get_assoc_context_var_name;
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
	auto assoc_ids = system_assoc_ids(sys_like_id);

	ctx.write("// Guaranteed order: ");
	ctx.write(cpp_codegen_plugin_util::comma_delim(
		assoc_ids | std::views::transform([&](auto assoc_id) -> std::string {
			return get_assoc_context_type_name(sys_like_id, assoc_id);
		})
	));
	ctx.write("\n");
	ctx.write(std::format(
		"std::array<ecsact_system_execution_context*, {}> other_contexts;\n\n",
		assoc_ids.size()
	));
}

static auto push_back_unique(auto& vec, const auto& element) -> void {
	if(std::ranges::find(vec, element) == std::end(vec)) {
		vec.push_back(element);
	}
}

auto provider::association::before_make_view_or_group(
	codegen_plugin_context&   ctx,
	const common_vars&        names,
	std::vector<std::string>& additional_view_components
) -> void {
	// for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
	// 	auto assoc_caps =
	// 		ecsact::meta::system_assoc_capabilities(sys_like_id, assoc_id);
	// 	auto assoc_system_details =
	// 		ecsact_entt_system_details::from_capabilities(assoc_caps);
	//
	// 	for(auto compo_id : assoc_composites.at(assoc_id)) {
	// 		// TODO: At the time of writing this is safe. It's very possible we
	// 		// allow actions to be referenecd in association fields in the near
	// 		// future and at that point this must be addressed.
	// 		auto comp_like_id = static_cast<ecsact_component_like_id>(compo_id);
	// 		if(!assoc_system_details.get_comps.contains(comp_like_id)) {
	// 			auto comp_cpp_ident = cpp_identifier(decl_full_name(comp_like_id));
	// 			push_back_unique(additional_view_components, comp_cpp_ident);
	// 		}
	// 	}
	// }
}

auto provider::association::after_make_view_or_group(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> void {
}

auto provider::association::context_function_other(
	codegen_plugin_context& ctx,
	const common_vars&      names
) -> handle_exclusive_provide {
	context_other_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

template<typename CompositeID>
static auto get_assoc_fields(CompositeID compo_id
) -> std::vector<ecsact_field_id> {
	auto result = std::vector<ecsact_field_id>{};

	for(auto field_id : ecsact::meta::get_field_ids(compo_id)) {
		auto field_type = ecsact::meta::get_field_type(compo_id, field_id);
		if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN &&
			 field_type.type.builtin == ECSACT_ENTITY_TYPE) {
			result.push_back(field_id);
		}

		if(field_type.kind == ECSACT_TYPE_KIND_FIELD_INDEX) {
			result.push_back(field_id);
		}
	}

	return result;
}

static auto has_assoc_fields(ecsact::rt_entt_codegen::system_like_id_variant id
) -> bool {
	for(auto&& [comp_id, _] : ecsact::meta::system_capabilities_list(id)) {
		if(!get_assoc_fields(comp_id).empty()) {
			return true;
		}
	}
	return false;
}

static auto get_first_assoc_comp(
	ecsact::rt_entt_codegen::system_like_id_variant id
) -> std::optional<ecsact_component_like_id> {
	for(auto&& [comp_id, _] : ecsact::meta::system_capabilities_list(id)) {
		if(!get_assoc_fields(comp_id).empty()) {
			return comp_id;
		}
	}
	return {};
}

auto provider::association::entity_iteration(
	codegen_plugin_context& ctx,
	const common_vars&      names,
	std::function<void()>   iter_func
) -> handle_exclusive_provide {
	auto assoc_ids = ecsact::meta::system_assoc_ids(sys_like_id);

	auto assoc_multi_view_block =
		std::optional<cpp_codegen_plugin_util::block_printer>{};
	block(ctx, "for(auto entity : view)", [&] {
		if(has_assoc_fields(sys_like_id)) {
			ctx.write(std::format( //
				"for(auto storage_id : "
				"view.get<ecsact::entt::detail::multi_assoc_storage<{}>>(entity)."
				"storage_hash_value_ids)",
				cpp_identifier(decl_full_name(get_first_assoc_comp(sys_like_id).value())
				)
			));
			assoc_multi_view_block.emplace(ctx);
			ctx.write(std::format(
				"context.storage.c{} = &registry.storage<{}>(storage_id);",
				static_cast<int>(get_first_assoc_comp(sys_like_id).value()),
				cpp_identifier(decl_full_name(get_first_assoc_comp(sys_like_id).value())
				)
			));
		}

		for(auto assoc_id : assoc_ids) {
			auto assoc_comp =
				ecsact::meta::system_assoc_component_id(sys_like_id, assoc_id);
			auto assoc_comp_cpp_ident = cpp_identifier(decl_full_name(assoc_comp));
			auto assoc_caps =
				ecsact::meta::system_assoc_capabilities(sys_like_id, assoc_id);
			auto assoc_system_details =
				ecsact_entt_system_details::from_capabilities(assoc_caps);
			auto make_view_opts = util::make_view_options(assoc_system_details);
			make_view_opts.view_var_name = assoc_view_names.at(assoc_id);
			make_view_opts.registry_var_name = names.registry_var_name;

			push_back_unique(
				make_view_opts.additional_components,
				assoc_comp_cpp_ident
			);

			util::make_view(ctx, make_view_opts);
		}

		print_other_contexts(ctx, names);

		for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
			auto assoc_comp_id =
				ecsact::meta::system_assoc_component_id(sys_like_id, assoc_id);
			auto assoc_comp_cpp_ident = cpp_identifier(decl_full_name(assoc_comp_id));
			auto assoc_comp_field_ids =
				ecsact::meta::system_assoc_fields(sys_like_id, assoc_id);

			ctx.write(std::format(
				"{0}.storage({1}.storage<{2}>(static_cast<::entt::id_type>("
				"::ecsact::entt::detail::hash_vals32({2}::id, {3})"
				")));\n",
				assoc_view_names.at(assoc_id),
				names.registry_var_name,
				assoc_comp_cpp_ident,
				util::comma_delim(
					assoc_comp_field_ids |
					std::views::transform([&](auto field_id) -> std::string {
						return std::format(
							"context.storage.c{}->get(entity).{}",
							static_cast<int>(assoc_comp_id),
							ecsact::meta::field_name(assoc_comp_id, field_id)
						);
					})
				)
			));
		}

		for(auto assoc_index = 0; assoc_ids.size() > assoc_index; ++assoc_index) {
			auto assoc_id = assoc_ids.at(assoc_index);
			ctx.write(std::format(
				"auto {0}_itr = {0}.begin();\n",
				assoc_view_names.at(assoc_id)
			));

			ctx.write(std::format(
				"context.other_contexts[{}] = &{};\n",
				assoc_index,
				get_assoc_context_var_name(sys_like_id, assoc_id)
			));
		}

		block(ctx, "for(;;)", [&] {
			for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
				ctx.write(std::format(
					"if({0}_itr == {0}.end()) break;\n",
					assoc_view_names.at(assoc_id)
				));
			}

			for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
				ctx.write(std::format(
					"{}.entity = *{}_itr;\n",
					get_assoc_context_var_name(sys_like_id, assoc_id),
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
		auto struct_name = get_assoc_context_type_name(sys_like_id, assoc_id);
		auto context_name = get_assoc_context_var_name(sys_like_id, assoc_id);
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
			context_view_storage_struct_impl(ctx, assoc_caps);
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
		for(auto&& [comp_id, _] : assoc_caps) {
			auto comp_cpp_ident = cpp_identifier(decl_full_name(comp_id));
			ctx.write(std::format(
				"{}.storage.c{} = {}.storage<{}>();\n",
				context_name,
				static_cast<int>(comp_id),
				assoc_view_names.at(assoc_id),
				comp_cpp_ident
			));
		}
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
			.parameter(
				"ecsact::entt::detail::assoc_hash_value_t",
				"assoc_fields_hash"
			)
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
			.parameter(
				"ecsact::entt::detail::assoc_hash_value_t",
				"assoc_fields_hash"
			)
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
			.parameter(
				"ecsact::entt::detail::assoc_hash_value_t",
				"assoc_fields_hash"
			)
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
			.parameter(
				"ecsact::entt::detail::assoc_hash_value_t",
				"assoc_fields_hash"
			)
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
			.parameter("ecsact_system_assoc_id", "assoc_id")
			.return_type("ecsact_system_execution_context* final");

	context_other_impl(ctx, sys_like_id, details);
}
