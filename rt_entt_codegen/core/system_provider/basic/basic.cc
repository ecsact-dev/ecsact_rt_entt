#include "basic.hh"

#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/core/system_provider/system_ctx_functions.hh"

using ecsact::cc_lang_support::cpp_identifier;
using ecsact::meta::decl_full_name;

using namespace ecsact::rt_entt_codegen::core;

auto provider::basic::initialization(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	sys_caps = ecsact::meta::system_capabilities(sys_like_id);

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
	auto sys_caps = ecsact::meta::system_capabilities(sys_like_id);

	context_add_impl(ctx, sys_caps);
	return HANDLED;
}

auto provider::basic::context_function_remove(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	auto sys_caps = ecsact::meta::system_capabilities(sys_like_id);

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

auto provider::basic::entity_iteration(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::function<void()>                             iter_func
) -> handle_exclusive_provide {
	using ecsact::cpp_codegen_plugin_util::block;

	block(ctx, "for(ecsact::entt::entity_id entity : view)", [&] {
		auto assoc_multi_view_block =
			std::optional<cpp_codegen_plugin_util::block_printer>{};

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

		iter_func();
	});
	return HANDLED;
}

auto provider::basic::provide_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> handle_exclusive_provide {
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
	for(auto&& [comp_id, _] :
			ecsact::meta::system_capabilities_list(sys_like_id)) {
		// TODO: Replace this garbage code. just for POC
		if(get_assoc_fields(comp_id).empty()) {
			continue;
		}

		auto comp_cpp_ident = cpp_identifier(decl_full_name(comp_id));
		ctx.write(std::format(
			"context.storage.c{} = view.storage<{}>();\n",
			static_cast<int>(comp_id),
			comp_cpp_ident
		));
	}

	return HANDLED;
}

auto provider::basic::pre_exec_system_impl_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> void {
	ctx.write("context.entity = entity;\n");
}
