#include "core.hh"

#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <map>
#include <array>
#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/common.h"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "rt_entt_codegen/shared/comps_with_caps.hh"
#include "rt_entt_codegen/shared/sorting.hh"
#include "rt_entt_codegen/shared/system_util.hh"

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

template<typename T>
concept system_or_action =
	std::same_as<T, ecsact_system_id> || std::same_as<T, ecsact_action_id>;

using ecsact::rt_entt_codegen::system_comps_with_caps;
using ecsact::rt_entt_codegen::system_needs_sorted_entities;

template<system_or_action T>
static auto print_sys_exec_ctx_action(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	T                                                          system_id
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "action"}
			.parameter("void*", "out_action_data")
			.return_type("void final");

	if constexpr(std::is_same_v<T, ecsact_action_id>) {
		auto action_name = cpp_identifier(decl_full_name(system_id));

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

static auto print_sys_exec_ctx_add(
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

static auto print_sys_exec_ctx_remove(
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
			"::id));\n"
		);
		return;
	}
	block(
		ctx,
		"static const auto remove_fns = "
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_system_execution_context_remove)>",
		[&] {
			for(int i = 0; i < remove_comps.size(); ++i) {
				const auto comp_id = remove_comps[i];
				auto       type_name = cpp_identifier(decl_full_name(comp_id));
				ctx.write(
					"{ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), &wrapper::dynamic::context_remove<",
					type_name,
					"> },"
				);
			}
		}
	);
	ctx.write(";\n");
}

static auto print_sys_exec_ctx_get(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
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
	block(
		ctx,
		"static const auto get_fns = "
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_system_execution_context_get)>",
		[&] {
			for(const auto comp_id : details.readable_comps) {
				auto type_name = cpp_identifier(decl_full_name(comp_id));
				ctx.write(
					"{ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), &wrapper::dynamic::context_get<",
					type_name,
					"> },"
				);
			}
		}
	);
	ctx.write(";\n");

	ctx.write(
		"get_fns.at(component_id)(this, component_id, out_component_data);\n"
	);
}

static auto print_sys_exec_ctx_update(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
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
			"component_data); \n"
		);
		return;
	}
	block(
		ctx,
		"static const auto update_fns = "
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_system_execution_context_update)>",
		[&] {
			for(const auto comp_id : details.readable_comps) {
				auto type_name = cpp_identifier(decl_full_name(comp_id));
				ctx.write(
					"{ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), &wrapper::dynamic::context_update<",
					type_name,
					"> },"
				);
			}
		}
	);
	ctx.write(";\n");

	ctx.write("update_fns.at(component_id)(this, component_id, component_data);\n"
	);
}

static auto print_sys_exec_ctx_has(
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

static auto print_sys_exec_ctx_generate(
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

	for(auto sys_id : get_all_sorted_systems()) {
		auto sys_details = ecsact_entt_system_details::from_system_like(sys_id);
		auto system_sorting_struct_name = std::format(
			"::ecsact::entt::detail::system_sorted<{}>",
			cpp_identifier(decl_full_name(sys_id))
		);

		/* for(const auto& components : details.generate_comps) { */
		/*     auto matches = 0; */
		/* 	for(const auto& [comp_id, requirements] : components) { */
		/* 	} */
		/* } */
	}
}

static auto print_sys_exec_ctx_parent( //
	ecsact::codegen_plugin_context& ctx
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "parent"} //
			.return_type("const ecsact_system_execution_context* final");

	ctx.write("return this->parent_ctx;\n");
}

static auto print_sys_exec_ctx_other(
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

template<typename SystemLikeID>
	requires(!std::is_same_v<
					 ecsact_system_like_id,
					 std::remove_cvref_t<SystemLikeID>>)
struct print_ecsact_entt_system_details_options {
	SystemLikeID sys_like_id;
	std::string  system_name;
	std::string  registry_var_name;
	std::string  parent_context_var_name;
	/// only set if system is an action
	std::optional<std::string> action_var_name;
};

template<typename SystemLikeID>
	requires(!std::is_same_v<
						ecsact_system_like_id,
						std::remove_cvref_t<SystemLikeID>>)
static auto print_ecsact_entt_system_details(
	ecsact::codegen_plugin_context&                              ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&   details,
	const print_ecsact_entt_system_details_options<SystemLikeID> options
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::meta::get_child_system_ids;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;
	using ecsact::rt_entt_codegen::system_util::create_context_struct_name;
	using ecsact::rt_entt_codegen::system_util::create_context_var_name;
	using ecsact::rt_entt_codegen::system_util::is_notify_system;
	using ecsact::rt_entt_codegen::system_util::print_system_notify_views;
	using ecsact::rt_entt_codegen::util::method_printer;

	constexpr auto is_system_id =
		std::is_same_v<ecsact_system_id, std::remove_cvref_t<SystemLikeID>>;

	auto sys_caps = ecsact::meta::system_capabilities(options.sys_like_id);
	auto lazy_iteration_rate = 0;

	if constexpr(is_system_id) {
		lazy_iteration_rate = ecsact_meta_get_lazy_iteration_rate(
			static_cast<ecsact_system_id>(options.sys_like_id)
		);
	}
	auto exec_start_label_name =
		std::format("exec_start_{}_", static_cast<int>(options.sys_like_id));

	auto pending_lazy_exec_struct = std::format(
		"::ecsact::entt::detail::pending_lazy_execution<::{}>",
		options.system_name
	);

	auto system_sorting_struct_name = std::format(
		"::ecsact::entt::detail::system_sorted<{}>",
		options.system_name
	);

	auto additional_view_components = std::vector<std::string>{};

	if(lazy_iteration_rate > 0) {
		ctx.write(
			"constexpr auto lazy_iteration_rate_ = ",
			lazy_iteration_rate,
			";\n\n"
		);
		ctx.write("auto iteration_count_ = 0;\n\n");
		ctx.write(exec_start_label_name, ":\n");
		additional_view_components.push_back(pending_lazy_exec_struct);
	}

	constexpr auto is_system = std::is_same_v<
		std::remove_cvref_t<decltype(options.sys_like_id)>,
		ecsact_system_id>;

	if constexpr(is_system) {
		if(system_needs_sorted_entities(options.sys_like_id)) {
			additional_view_components.push_back(system_sorting_struct_name);
		}
	}

	if(is_notify_system(options.sys_like_id)) {
		additional_view_components.push_back(
			std::format("ecsact::entt::detail::run_system<{}>", options.system_name)
		);
		print_system_notify_views(
			ctx,
			details,
			options.sys_like_id,
			options.registry_var_name
		);
	}

	ecsact::rt_entt_codegen::util::make_view(
		ctx,
		"view",
		options.registry_var_name,
		details,
		additional_view_components
	);

	if constexpr(is_system) {
		if(system_needs_sorted_entities(options.sys_like_id)) {
			ctx.write("view.use<", system_sorting_struct_name, ">();\n");
		}
	}

	block(ctx, "struct : ecsact_system_execution_context ", [&] {
		ctx.write("decltype(view)* view;\n");

		ctx.write(
			"std::unordered_map<ecsact_entity_id,ecsact_system_execution_"
			"context*> "
			"other_contexts;\n\n"
		);

		if(options.action_var_name) {
			ctx.write("const void* action_data = nullptr;\n");
		}

		ctx.write("\n");
		print_sys_exec_ctx_action(ctx, details, options.sys_like_id);
		print_sys_exec_ctx_add(ctx, details, sys_caps);
		print_sys_exec_ctx_remove(ctx, details, sys_caps);
		print_sys_exec_ctx_get(ctx, details);
		print_sys_exec_ctx_update(ctx, details);
		print_sys_exec_ctx_has(ctx, details);
		print_sys_exec_ctx_generate(ctx, details);
		print_sys_exec_ctx_parent(ctx);
		print_sys_exec_ctx_other(ctx, details);
	});
	ctx.write("context;\n\n");

	ctx.write("context.registry = &", options.registry_var_name, ";\n");
	if(options.action_var_name) {
		ctx.write("context.action_data = ", *options.action_var_name, ";\n\n");
	}

	ctx.write(
		"context.id = ecsact_id_cast<ecsact_system_like_id>(::",
		options.system_name,
		"::id);\n"
	);
	ctx.write("context.parent_ctx = ", options.parent_context_var_name, ";\n");
	ctx.write("context.view = &view;\n\n");

	auto other_view_names = print_other_contexts(ctx, details, options);

	block(ctx, "for(ecsact::entt::entity_id entity : view)", [&] {
		if(lazy_iteration_rate > 0) {
			block(ctx, "if(iteration_count_ == lazy_iteration_rate_)", [&] {
				ctx.write("break;\n");
			});

			ctx.write("++iteration_count_;\n");
			ctx.write(
				options.registry_var_name,
				".erase<",
				pending_lazy_exec_struct,
				">(entity);\n"
			);
		}

		// value = comp var name
		auto components_with_entity_fields =
			std::map<ecsact_component_like_id, std::string>{};

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

		ctx.write("context.entity = entity;\n");

		for(auto child_sys_id : get_child_system_ids(options.sys_like_id)) {
			auto child_details = ecsact_entt_system_details::from_system_like(
				ecsact_id_cast<ecsact_system_like_id>(child_sys_id)
			);

			auto child_system_name = cpp_identifier(decl_full_name(child_sys_id));

			ctx.write(
				"ecsact::entt::execute_system<::" + child_system_name + ">(",
				options.registry_var_name,
				", &context);\n"
			);
		}
		ctx.write("\n");

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

		if(other_view_names.empty()) {
			ctx.write("system_impl(&context);\n");
		} else {
			// we need to check if we found any invalid associations
			block(
				ctx,
				"if(found_assoc_entities == " +
					std::to_string(other_view_names.size()) + ")",
				[&] { ctx.write("system_impl(&context);\n"); }
			);
		}

		ctx.write("\n");
	});

	if(lazy_iteration_rate > 0) {
		ctx.write(
			"// If this assertion triggers that's a ecsact_rt_entt codegen "
			"failure\n"
		);
		ctx.write("assert(iteration_count_ <= lazy_iteration_rate_);\n");
		block(ctx, "if(iteration_count_ < lazy_iteration_rate_)", [&] {
			ctx.write(
				"_recalc_sorting_hash<",
				options.system_name,
				">(",
				options.registry_var_name,
				");\n"
			);
			ctx.write(
				options.registry_var_name,
				".sort<",
				system_sorting_struct_name,
				">([](const auto& a, const auto& b) { return a.hash < b.hash; });\n"
			);

			ecsact::rt_entt_codegen::util::make_view(
				ctx,
				"view_no_pending_lazy_",
				options.registry_var_name,
				details
			);

			ctx.write("auto view_no_pending_lazy_count_ = 0;\n");

			block(
				ctx,
				"for(ecsact::entt::entity_id entity : view_no_pending_lazy_)",
				[&] {
					ctx.write(
						"// If this assertion triggers this is an indicator of a codegen "
						"failure.\n"
						"// Please report to https://github.com/ecsact-dev/ecsact_rt_entt\n"
					);
					ctx.write(
						"assert(",
						options.registry_var_name,
						".all_of<",
						system_sorting_struct_name,
						">(entity));\n"
					);
					ctx.write("view_no_pending_lazy_count_ += 1;\n");
					ctx.write(
						options.registry_var_name,
						".emplace<",
						pending_lazy_exec_struct,
						">(entity);\n"
					);
				}
			);

			block(
				ctx,
				"if(view_no_pending_lazy_count_ >= lazy_iteration_rate_)",
				[&] { ctx.write("goto ", exec_start_label_name, ";\n"); }
			);
		});
	}

	print_apply_pendings(
		ctx,
		details,
		options.sys_like_id,
		options.registry_var_name
	);
}

template<typename SystemLikeID>
static auto print_other_contexts(
	ecsact::codegen_plugin_context&                              ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details&   details,
	const print_ecsact_entt_system_details_options<SystemLikeID> options
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

		block(ctx, "struct " + struct_header, [&] {
			using namespace std::string_literals;
			using ecsact::rt_entt_codegen::util::decl_cpp_ident;
			using std::views::transform;

			ctx.write("decltype(", view_name, ")* view;\n");
			ctx.write("\n");
			print_sys_exec_ctx_action(ctx, other_details, options.sys_like_id);
			print_sys_exec_ctx_add(ctx, other_details, assoc_detail.capabilities);
			print_sys_exec_ctx_remove(ctx, other_details, assoc_detail.capabilities);
			print_sys_exec_ctx_get(ctx, other_details);
			print_sys_exec_ctx_update(ctx, other_details);
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
					"ecsact::entt::entity_id(entity));\n"
				);
			}
		}
	});
	ctx.write("\n");
	print_apply_pendings(ctx, details, system_like_id, "registry");
}

static auto print_execute_system_template_specialization(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	ecsact_system_id                                           system_id
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::system_util::is_trivial_system;

	using ecsact::rt_entt_codegen::util::method_printer;

	if(is_trivial_system(ecsact_id_cast<ecsact_system_like_id>(system_id))) {
		print_trivial_system_like(
			ctx,
			details,
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
			.return_type("void");

	ctx.write(
		"auto system_impl = ::ecsact::entt::get_system_impl<::",
		system_name,
		">();\n"
	);

	block(ctx, "if(system_impl == nullptr)", [&] { ctx.write("return;"); });

	print_ecsact_entt_system_details<ecsact_system_id>(
		ctx,
		details,
		{
			.sys_like_id = system_id,
			.system_name = system_name,
			.registry_var_name = "registry",
			.parent_context_var_name = "parent_context",
			.action_var_name = std::nullopt,
		}
	);
}

static auto print_execute_actions_template_specialization(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	ecsact_action_id                                           action_id
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::system_util::is_trivial_system;
	using ecsact::rt_entt_codegen::util::method_printer;

	if(is_trivial_system(ecsact_id_cast<ecsact_system_like_id>(action_id))) {
		print_trivial_system_like(
			ctx,
			details,
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
			.parameter("std::span<::" + cpp_system_name + " const*>", "actions")
			.return_type("void");

	ctx.write(
		"auto system_impl = ::ecsact::entt::get_system_impl<::",
		cpp_system_name,
		">();\n"
	);

	block(ctx, "if(system_impl == nullptr)", [&] { ctx.write("return;"); });

	block(ctx, "for(auto action : actions)", [&] {
		print_ecsact_entt_system_details<ecsact_action_id>(
			ctx,
			details,
			{
				.sys_like_id = action_id,
				.system_name = cpp_system_name,
				.registry_var_name = "registry",
				.parent_context_var_name = "nullptr",
				.action_var_name = "action",
			}
		);
	});
}

template<typename SystemLikeID>
static auto print_child_execution_system_like_template_specializations(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	SystemLikeID                                               sys_like_id
) -> void {
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;

	for(auto child_sys_id : ecsact::meta::get_child_system_ids(sys_like_id)) {
		auto sys_details = ecsact_entt_system_details::from_system_like(
			static_cast<ecsact_system_like_id>(child_sys_id)
		);

		print_execute_system_template_specialization(
			ctx,
			sys_details,
			static_cast<ecsact_system_id>(child_sys_id)
		);

		print_child_execution_system_like_template_specializations(
			ctx,
			sys_details,
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
		auto sys_details =
			ecsact_entt_system_details::from_system_like(sys_like_id);

		print_child_execution_system_like_template_specializations(
			ctx,
			sys_details,
			sys_like_id
		);

		if(details.is_system(sys_like_id)) {
			print_execute_system_template_specialization(
				ctx,
				sys_details,
				static_cast<ecsact_system_id>(sys_like_id)
			);
		} else if(details.is_action(sys_like_id)) {
			print_execute_actions_template_specialization(
				ctx,
				sys_details,
				static_cast<ecsact_action_id>(sys_like_id)
			);
		}
	}
}
