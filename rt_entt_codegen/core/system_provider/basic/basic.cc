#include "basic.hh"

#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using ecsact::cc_lang_support::cpp_identifier;
using ecsact::cpp_codegen_plugin_util::block;
using ecsact::meta::decl_full_name;
using ecsact::rt_entt_codegen::util::is_transient_component;

auto ecsact::rt_entt_codegen::core::provider::basic::initialization(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> void {
	sys_caps =
		ecsact::meta::system_capabilities(sys_like_id_variant.get_sys_like_id());

	view_type_name = "view";
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_action(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	if(sys_like_id_variant.is_action()) {
		auto action_name =
			cpp_identifier(decl_full_name(sys_like_id_variant.get_sys_like_id()));

		ctx.write(
			"*static_cast<",
			action_name,
			"*>(out_action_data) = *static_cast<const ",
			action_name,
			"*>(action_data);\n"
		);
	} else {
		// TODO(Kelwan): Trying to access .action() without a valid action
		ctx.write("\n");
	}
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_add(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	auto adds_comps = std::vector<ecsact_component_like_id>{};

	for(auto&& [comp_id, sys_cap] : sys_caps) {
		if(is_transient_component(ctx.package_id, comp_id)) {
			continue;
		}
		if((ECSACT_SYS_CAP_ADDS & sys_cap) == ECSACT_SYS_CAP_ADDS) {
			adds_comps.push_back(comp_id);
		}
	}

	if(adds_comps.empty()) {
		// TODO(Kelwan): Handle unexpected behaviour
		return HANDLED;
	} else if(adds_comps.size() == 1) {
		const auto& comp_id = adds_comps.front();
		auto        type_name = cpp_identifier(decl_full_name(comp_id));
		ctx.write(
			"wrapper::dynamic::context_add<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_like_id>(",
			type_name,
			"::id),",
			"component_data); \n"
		);
		return HANDLED;
	}
	block(
		ctx,
		"static const auto add_fns = "
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_system_execution_context_add)>",
		[&] {
			for(int i = 0; i < adds_comps.size(); ++i) {
				const auto comp_id = adds_comps[i];
				auto       type_name = cpp_identifier(decl_full_name(comp_id));
				ctx.write(
					"{",
					"ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), ",
					"&wrapper::dynamic::context_add<::",
					type_name,
					"> },"
				);
			}
		}
	);

	ctx.write(";\n");

	ctx.write("add_fns.at(component_id)(this, component_id, component_data);\n");

	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_remove(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	auto remove_comps = std::vector<ecsact_component_like_id>{};

	for(auto&& [comp_id, sys_cap] : sys_caps) {
		if(is_transient_component(ctx.package_id, comp_id)) {
			continue;
		}
		if((ECSACT_SYS_CAP_REMOVES & sys_cap) == ECSACT_SYS_CAP_REMOVES) {
			remove_comps.push_back(comp_id);
		}
	}

	if(remove_comps.size() == 0) {
		// TODO(Kelwan): Handle unexpected behaviour
		return HANDLED;
	}
	if(remove_comps.size() == 1) {
		const auto& comp_id = remove_comps.front();

		auto type_name = cpp_identifier(decl_full_name(comp_id));
		ctx.write(
			"wrapper::dynamic::context_remove<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_like_id>(",
			type_name,
			"::id), *view);\n"
		);
		return HANDLED;
	}
	ctx.write(std::format(
		"using remove_fn_t = void (*)(ecsact_system_execution_context*, "
		"ecsact_component_like_id, {}_t&);\n",
		view_type_name
	));

	ctx.write("static const auto remove_fns = []()\n");

	block(ctx, "", [&] {
		ctx.write(
			"auto result = std::unordered_map<ecsact_component_like_id, "
			"remove_fn_t>{};\n"
		);
		for(const auto comp_id : details.removable_comps) {
			auto type_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result[ecsact_id_cast<ecsact_component_like_id>(",
				type_name,
				"::id)] = &wrapper::dynamic::context_remove<",
				type_name,
				">;\n"
			);
		}

		ctx.write("return result;\n");
	});
	ctx.write("();\n");
	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_get(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	if(details.readable_comps.size() == 0) {
		return HANDLED;
	}

	std::vector<ecsact_component_like_id> get_components;

	for(auto comp_id : details.readable_comps) {
		if(is_transient_component(ctx.package_id, comp_id)) {
			continue;
		}
		get_components.insert(get_components.end(), comp_id);
	}

	if(get_components.size() == 0) {
		return HANDLED;
	}

	// Shortcut - ignore component ID because we only have 1
	if(details.get_comps.size() == 1 && details.readable_comps.size() == 1) {
		auto comp_id = *details.get_comps.begin();
		auto cpp_comp_full_name = cpp_identifier(decl_full_name(comp_id));

		ctx.write(
			"assert(ecsact_id_cast<ecsact_component_like_id>(::",
			cpp_comp_full_name,
			"::id) == component_id);\n"
		);
		ctx.write(
			"*static_cast<::",
			cpp_comp_full_name,
			"*>(out_component_data) = view->get<::",
			cpp_comp_full_name,
			">(entity);"
		);
		return HANDLED;
	}

	ctx.write(std::format(
		"using get_fn_t = void (*)(ecsact_system_execution_context*, "
		"ecsact_component_like_id, void *, {}_t&);\n",
		view_type_name
	));

	ctx.write("static const auto get_fns = []()\n");

	block(ctx, "", [&] {
		ctx.write(
			"auto result = std::unordered_map<ecsact_component_like_id, "
			"get_fn_t>{};\n"
		);
		for(const auto comp_id : details.readable_comps) {
			auto type_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result[ecsact_id_cast<ecsact_component_like_id>(",
				type_name,
				"::id)] = &wrapper::dynamic::context_get<",
				type_name,
				">;\n"
			);
		}

		ctx.write("return result;\n");
	});
	ctx.write("();\n");

	ctx.write(
		"get_fns.at(component_id)(this, component_id, out_component_data, *view);\n"
	);

	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_update(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	if(details.writable_comps.size() == 0) {
		return HANDLED;
	}

	if(details.writable_comps.size() == 1) {
		const auto& comp_id = *details.writable_comps.begin();
		auto        type_name = cpp_identifier(decl_full_name(comp_id));
		ctx.write(
			"wrapper::dynamic::context_update<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_like_id>(",
			type_name,
			"::id),",
			"component_data, *view); \n"
		);
		return HANDLED;
	}

	ctx.write(std::format(
		"using update_fn_t = void (*)(ecsact_system_execution_context*, "
		"ecsact_component_like_id, const void *, {}_t&);\n",
		view_type_name
	));

	ctx.write("static const auto update_fns = []()\n");

	block(ctx, "", [&] {
		ctx.write(
			"auto result = std::unordered_map<ecsact_component_like_id, "
			"update_fn_t>{};\n"
		);
		for(const auto comp_id : details.writable_comps) {
			auto type_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result[ecsact_id_cast<ecsact_component_like_id>(",
				type_name,
				"::id)] = &wrapper::dynamic::context_update<",
				type_name,
				">;\n"
			);
		}

		ctx.write("return result;\n");
	});
	ctx.write("();\n");

	ctx.write(
		"update_fns.at(component_id)(this, component_id, component_data, "
		"*view);\n"
	);

	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_has(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	if(details.writable_comps.size() == 0) {
		ctx.write("return false;");
		return HANDLED;
	}

	if(details.writable_comps.size() == 1) {
		const auto& comp_id = *details.writable_comps.begin();
		auto        type_name = cpp_identifier(decl_full_name(comp_id));
		ctx.write(
			"return wrapper::dynamic::context_has<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_like_id>(",
			type_name,
			"::id));\n"
		);
	}
	block(
		ctx,
		"static const auto has_fns = "
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_system_execution_context_has)>",
		[&] {
			for(const auto comp_id : details.readable_comps) {
				auto type_name = cpp_identifier(decl_full_name(comp_id));
				ctx.write(
					"{ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), &wrapper::dynamic::context_has<",
					type_name,
					"> },"
				);
			}
		}
	);
	ctx.write(";\n");

	ctx.write("return has_fns.at(component_id)(this, component_id);\n");

	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_generate(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	if(details.generate_comps.size() == 0) {
		// TODO (Kelwan): Handle undefined behaviour
		return HANDLED;
	}
	block(
		ctx,
		"static const auto generate_fns = "
		"std::unordered_map<ecsact_component_id, "
		"void(*)(ecsact_system_execution_context*, ecsact_component_id, const "
		"void*, ecsact::entt::entity_id)>",
		[&] {
			for(const auto& component : details.generate_comps) {
				for(const auto& [comp_id, requirements] : component) {
					auto type_name = cpp_identifier(decl_full_name(comp_id));
					ctx.write(
						"{",
						type_name,
						"::id, &wrapper::dynamic::context_generate_add<",
						type_name,
						"> },"
					);
				}
			}
		}
	);
	ctx.write(";\n");

	// NOTE(Kelwan): Multiple generates blocks are allowed in Ecsact systems but
	// currently the interpreter won't allow this. More testing required after the
	// issue is resolved https://github.com/ecsact-dev/ecsact_interpret/issues/185
	ctx.write("auto entity = registry->create();\n");

	ctx.write(
		"registry->template emplace<ecsact::entt::detail::created_entity>(entity, "
		"ecsact_generated_entity);\n"
	);

	block(ctx, "for(int i = 0; i < component_count; ++i)", [&] {
		ctx.write("const auto component_id = component_ids[i];\n");
		ctx.write("const void* component_data = components_data[i];\n");

		ctx.write(
			"generate_fns.at(component_id)(this, component_id, "
			"component_data, entity);\n"
		);
	});

	return HANDLED;
}

auto ecsact::rt_entt_codegen::core::provider::basic::context_function_parent(
	ecsact::codegen_plugin_context&                                       ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&            details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names& names
) -> handle_exclusive_provide {
	if(details.association_details.size() == 0) {
		// TODO(Kelwan): Handle undefined behaviour
		// Attempt to access other without association

		ctx.write("return nullptr;");
		return HANDLED;
	}

	ctx.write(
		"if(other_contexts.contains(entity)) {\n",
		"return other_contexts.at(entity);\n}\n"
	);
	// NOTE(Kelwan): Maybe we handle undefined behaviour here too
	ctx.write("return nullptr;");

	return HANDLED;
}
