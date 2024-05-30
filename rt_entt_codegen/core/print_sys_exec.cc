#include "core.hh"

#include <string>
#include <unordered_map>
#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/common.h"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "rt_entt_codegen/shared/comps_with_caps.hh"
#include "rt_entt_codegen/shared/sorting.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/shared/parallel.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "system_provider/system_provider.hh"
#include "rt_entt_codegen/core/system_provider/lazy/lazy.hh"
#include "system_provider/lazy/lazy.hh"
#include "system_provider/association/association.hh"
#include "system_provider/notify/notify.hh"

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

template<typename T>
concept system_or_action =
	std::same_as<T, ecsact_system_id> || std::same_as<T, ecsact_action_id>;

using ecsact::rt_entt_codegen::system_comps_with_caps;
using ecsact::rt_entt_codegen::core::provider::system_like_id_variant;

auto print_sys_exec_ctx_action(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&           details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "action"}
			.parameter("void*", "out_action_data")
			.return_type("void final");

	if(options.sys_like_id_variant.is_action()) {
		auto action_name = cpp_identifier(
			decl_full_name(options.sys_like_id_variant.get_sys_like_id())
		);

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
}

auto print_sys_exec_ctx_add(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	capability_t                                               sys_caps
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::is_transient_component;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "add"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("const void*", "component_data")
			.return_type("void final");

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
		return;
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
		return;
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
}

auto print_sys_exec_ctx_remove(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	capability_t                                               sys_caps,
	const std::string&                                         view_type_name
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::is_transient_component;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "remove"}
			.parameter("ecsact_component_like_id", "component_id")
			.return_type("void final");

	auto remove_comps = std::vector<ecsact_component_like_id>{};

	for(auto&& [comp_id, sys_cap] : sys_caps) {
		if(is_transient_component(ctx.package_id, comp_id)) {
			continue;
		}
		if((ECSACT_SYS_CAP_REMOVES & sys_cap) == ECSACT_SYS_CAP_REMOVES) {
			remove_comps.push_back(comp_id);
		}
	}

	ctx.write("// Size: ", remove_comps.size(), "\n");

	if(remove_comps.size() == 0) {
		// TODO(Kelwan): Handle unexpected behaviour
		return;
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
		return;
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
}

auto print_sys_exec_ctx_get(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::is_transient_component;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "get"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("void*", "out_component_data")
			.return_type("void final");

	if(details.readable_comps.size() == 0) {
		return;
	}

	std::vector<ecsact_component_like_id> get_components;

	for(auto comp_id : details.readable_comps) {
		if(is_transient_component(ctx.package_id, comp_id)) {
			continue;
		}
		get_components.insert(get_components.end(), comp_id);
	}

	if(get_components.size() == 0) {
		return;
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
		return;
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
}

auto print_sys_exec_ctx_update(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "update"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("const void*", "component_data")
			.return_type("void final");

	if(details.writable_comps.size() == 0) {
		return;
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
		return;
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
}

auto print_sys_exec_ctx_has(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	// use optional_comps

	auto printer = //
		method_printer{ctx, "has"}
			.parameter("ecsact_component_like_id", "component_id")
			.return_type("bool final");

	if(details.writable_comps.size() == 0) {
		ctx.write("return false;");
		return;
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
}

auto print_sys_exec_ctx_generate(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;
	using ecsact::rt_entt_codegen::get_all_sorted_systems;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "generate"}
			.parameter("int", "component_count")
			.parameter("ecsact_component_id*", "component_ids")
			.parameter("const void**", "components_data")
			.return_type("void final");

	if(details.generate_comps.size() == 0) {
		// TODO (Kelwan): Handle undefined behaviour
		return;
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
}

auto print_sys_exec_ctx_parent( //
	ecsact::codegen_plugin_context& ctx
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "parent"} //
			.return_type("const ecsact_system_execution_context* final");

	ctx.write("return this->parent_ctx;\n");
}

auto print_sys_exec_ctx_other(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "other"}
			.parameter("ecsact_entity_id", "entity")
			.return_type("ecsact_system_execution_context* final");

	if(details.association_details.size() == 0) {
		// TODO(Kelwan): Handle undefined behaviour
		// Attempt to access other without association

		ctx.write("return nullptr;");
		return;
	}

	ctx.write(
		"if(other_contexts.contains(entity)) {\n",
		"return other_contexts.at(entity);\n}\n"
	);
	// NOTE(Kelwan): Maybe we handle undefined behaviour here too
	ctx.write("return nullptr;");
}

template<typename SystemLikeID>
static auto print_apply_pendings(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	SystemLikeID                                               sys_like_id,
	std::string                                                registry_name
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;

	auto add_comps = system_comps_with_caps(sys_like_id, ECSACT_SYS_CAP_ADDS);

	for(auto comp_id : add_comps) {
		auto comp_name = cpp_identifier(decl_full_name(comp_id));

		ctx.write(
			"ecsact::entt::detail::apply_pending_add<",
			comp_name,
			">(",
			registry_name,
			");\n"
		);
	}

	auto remove_comps =
		system_comps_with_caps(sys_like_id, ECSACT_SYS_CAP_REMOVES);

	for(auto comp_id : remove_comps) {
		auto comp_name = cpp_identifier(decl_full_name(comp_id));

		ctx.write(
			"ecsact::entt::detail::apply_pending_remove<",
			comp_name,
			">(",
			registry_name,
			");\n"
		);
	}

	for(auto generate_details : details.generate_comps) {
		for(auto&& [comp_id, generate] : generate_details) {
			auto comp_name = cpp_identifier(decl_full_name(comp_id));

			ctx.write(
				"ecsact::entt::detail::apply_pending_add<",
				comp_name,
				">(",
				registry_name,
				");\n"
			);
		}
	}
}

static auto print_execute_systems(
	ecsact::codegen_plugin_context& ctx,
	system_like_id_variant          sys_like_id_variant,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& sys_details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names
) -> void {
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::rt_entt_codegen::core::provider::association;
	using ecsact::rt_entt_codegen::core::provider::lazy;
	using ecsact::rt_entt_codegen::core::provider::notify;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto lazy_provider = lazy{sys_like_id_variant};

	auto association_provider = association{sys_like_id_variant};
	auto notify_provider = notify{sys_like_id_variant};

	auto sys_caps =
		ecsact::meta::system_capabilities(sys_like_id_variant.get_sys_like_id());

	auto additional_view_components = std::vector<std::string>{};

	lazy_provider.before_make_view_or_group(
		ctx,
		sys_details,
		names,
		additional_view_components
	);
	association_provider.before_make_view_or_group(
		ctx,
		sys_details,
		names,
		additional_view_components
	);
	notify_provider.before_make_view_or_group(
		ctx,
		sys_details,
		names,
		additional_view_components
	);

	ecsact::rt_entt_codegen::util::make_view(
		ctx,
		"view",
		names.registry_var_name,
		sys_details,
		additional_view_components
	);

	ctx.write("using view_t = decltype(view);\n");

	lazy_provider.after_make_view_or_group(ctx, sys_details, names);

	block(ctx, "struct : ecsact_system_execution_context ", [&] {
		ctx.write("view_t* view;\n");

		association_provider.context_function_header(ctx, sys_details, names);

		if(names.action_var_name) {
			ctx.write("const void* action_data = nullptr;\n");
		}

		ctx.write("\n");
		print_sys_exec_ctx_action(ctx, sys_details, names);
		print_sys_exec_ctx_add(ctx, sys_details, sys_caps);
		print_sys_exec_ctx_remove(ctx, sys_details, sys_caps, "view");
		print_sys_exec_ctx_get(ctx, sys_details, "view");
		print_sys_exec_ctx_update(ctx, sys_details, "view");
		print_sys_exec_ctx_has(ctx, sys_details);
		print_sys_exec_ctx_generate(ctx, sys_details);
		print_sys_exec_ctx_parent(ctx);
		print_sys_exec_ctx_other(ctx, sys_details);
	});
	ctx.write("context;\n\n");

	ctx.write("context.registry = &", names.registry_var_name, ";\n");
	if(names.action_var_name) {
		ctx.write("context.action_data = ", *names.action_var_name, ";\n\n");
	}

	ctx.write(
		"context.id = ecsact_id_cast<ecsact_system_like_id>(::",
		names.system_name,
		"::id);\n"
	);
	ctx.write("context.parent_ctx = ", names.parent_context_var_name, ";\n");
	ctx.write("context.view = &view;\n\n");

	association_provider.pre_entity_iteration(ctx, sys_details, names);

	block(ctx, "for(ecsact::entt::entity_id entity : view)", [&] {
		ctx.write("context.entity = entity;\n");
		lazy_provider.pre_exec_system_impl(ctx, sys_details, names);
		association_provider.pre_exec_system_impl(ctx, sys_details, names);

		association_provider.system_impl(ctx, sys_details, names);

		ctx.write("\n");
	});

	lazy_provider.post_exec_system_impl(ctx, sys_details, names);

	print_apply_pendings(
		ctx,
		sys_details,
		sys_like_id_variant.get_sys_like_id(),
		names.registry_var_name
	);
}

static auto print_trivial_system_like(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	ecsact_system_like_id                                      system_like_id
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;

	using ecsact::meta::decl_full_name;
	using ecsact::meta::get_field_ids;
	using ecsact::meta::system_capabilities;

	using ecsact::rt_entt_codegen::util::method_printer;

	auto system_name = cpp_identifier(decl_full_name(system_like_id));

	auto sys_capabilities = system_capabilities(system_like_id);

	ctx.write("template<>\n");
	auto printer = //
		method_printer{ctx, "ecsact::entt::execute_system<::" + system_name + ">"}
			.parameter("::entt::registry&", "registry")
			.parameter("ecsact_system_execution_context*", "parent_context")
			.parameter("const ecsact::entt::actions_map&", "actions_map")
			.return_type("void");

	ecsact::rt_entt_codegen::util::make_view(ctx, "view", "registry", details);

	block(ctx, "for(auto entity : view)", [&] {
		for(auto&& [comp_id, capability] : sys_capabilities) {
			auto type_name = cpp_identifier(decl_full_name(comp_id));

			if((ECSACT_SYS_CAP_ADDS & capability) == ECSACT_SYS_CAP_ADDS) {
				ctx.write(
					"ecsact::entt::wrapper::dynamic::component_add_trivial<",
					type_name,
					">(registry, "
					"ecsact::entt::entity_id(entity);\n"
				);
			}
			if((ECSACT_SYS_CAP_REMOVES & capability) == ECSACT_SYS_CAP_REMOVES) {
				ctx.write(
					"ecsact::entt::wrapper::dynamic::component_remove_trivial<",
					type_name,
					">(registry, "
					"ecsact::entt::entity_id(entity), view);\n"
				);
			}
		}
	});
	ctx.write("\n");
	print_apply_pendings(ctx, details, system_like_id, "registry");
}

static auto print_execute_system_template_specialization(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details,
	ecsact_system_id                                    system_id
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;
	using ecsact::rt_entt_codegen::system_util::is_trivial_system;

	using ecsact::rt_entt_codegen::util::method_printer;

	auto sys_details = ecsact_entt_system_details::from_system_like(
		ecsact_id_cast<ecsact_system_like_id>(system_id)
	);

	if(is_trivial_system(ecsact_id_cast<ecsact_system_like_id>(system_id))) {
		print_trivial_system_like(
			ctx,
			sys_details,
			ecsact_id_cast<ecsact_system_like_id>(system_id)
		);
		return;
	}
	auto system_name = cpp_identifier(decl_full_name(system_id));

	ctx.write("template<>\n");
	auto printer = //
		method_printer{ctx, "ecsact::entt::execute_system<::" + system_name + ">"}
			.parameter("::entt::registry&", "registry")
			.parameter("ecsact_system_execution_context*", "parent_context")
			.parameter("const ecsact::entt::actions_map&", "actions_map")
			.return_type("void");

	ctx.write(
		"auto system_impl = ::ecsact::entt::get_system_impl<::",
		system_name,
		">();\n"
	);

	block(ctx, "if(system_impl == nullptr)", [&] { ctx.write("return;"); });

	print_execute_systems(
		ctx,
		system_id,
		sys_details,
		{
			.registry_var_name = "registry",
			.parent_context_var_name = "parent_context",
			.action_var_name = std::nullopt,
		}
	);
}

static auto print_execute_actions_template_specialization(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details,
	ecsact_action_id                                    action_id
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;
	using ecsact::rt_entt_codegen::system_util::is_trivial_system;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto sys_details = ecsact_entt_system_details::from_system_like(
		ecsact_id_cast<ecsact_system_like_id>(action_id)
	);

	if(is_trivial_system(ecsact_id_cast<ecsact_system_like_id>(action_id))) {
		print_trivial_system_like(
			ctx,
			sys_details,
			ecsact_id_cast<ecsact_system_like_id>(action_id)
		);
		return;
	}

	auto cpp_system_name = cpp_identifier(decl_full_name(action_id));

	const auto method_name =
		"ecsact::entt::execute_actions<::" + cpp_system_name + ">";

	ctx.write("template<>\n");
	auto printer = //
		method_printer{ctx, method_name}
			.parameter("::entt::registry&", "registry")
			.parameter("ecsact_system_execution_context", "*parent_context")
			.parameter("const ecsact::entt::actions_map&", "actions_map")
			.return_type("void");

	ctx.write(
		"auto system_impl = ::ecsact::entt::get_system_impl<::",
		cpp_system_name,
		">();\n"
	);

	block(ctx, "if(system_impl == nullptr)", [&] { ctx.write("return;"); });

	ctx.write(
		"auto actions = actions_map.as_action_span<",
		cpp_system_name,
		">();\n"
	);

	block(ctx, "for(auto action : actions)", [&] {
		print_execute_systems(
			ctx,
			action_id,
			sys_details,
			{
				.registry_var_name = "registry",
				.parent_context_var_name = "nullptr",
				.action_var_name = "action",
			}
		);
	});
}

template<typename SystemLikeID>
static auto print_child_execution_system_like_template_specializations(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details,
	SystemLikeID                                        sys_like_id
) -> void {
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;

	for(auto child_sys_id : ecsact::meta::get_child_system_ids(sys_like_id)) {
		print_execute_system_template_specialization(
			ctx,
			details,
			static_cast<ecsact_system_id>(child_sys_id)
		);

		print_child_execution_system_like_template_specializations(
			ctx,
			details,
			static_cast<ecsact_system_like_id>(child_sys_id)
		);
	}
}

auto ecsact::rt_entt_codegen::core::
	print_execute_system_like_template_specializations( //
		codegen_plugin_context&    ctx,
		const ecsact_entt_details& details
	) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;
	using ecsact::rt_entt_codegen::util::method_printer;

	for(auto sys_like_id : details.top_execution_order) {
		print_child_execution_system_like_template_specializations(
			ctx,
			details,
			sys_like_id
		);

		if(details.is_system(sys_like_id)) {
			print_execute_system_template_specialization(
				ctx,
				details,
				static_cast<ecsact_system_id>(sys_like_id)
			);
		} else if(details.is_action(sys_like_id)) {
			print_execute_actions_template_specialization(
				ctx,
				details,
				static_cast<ecsact_action_id>(sys_like_id)
			);
		}
	}
}
