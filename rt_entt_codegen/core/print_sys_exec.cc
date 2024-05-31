#include "core.hh"

#include <string>
#include <unordered_map>
#include <memory>
#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/common.h"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "rt_entt_codegen/shared/comps_with_caps.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "system_provider/system_provider.hh"
#include "rt_entt_codegen/core/system_provider/lazy/lazy.hh"
#include "system_provider/lazy/lazy.hh"
#include "system_provider/association/association.hh"
#include "system_provider/notify/notify.hh"
#include "system_provider/basic/basic.hh"

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

template<typename T>
concept system_or_action =
	std::same_as<T, ecsact_system_id> || std::same_as<T, ecsact_action_id>;

using ecsact::rt_entt_codegen::system_comps_with_caps;
using ecsact::rt_entt_codegen::system_like_id_variant;
using ecsact::rt_entt_codegen::core::provider::system_provider;

auto print_sys_exec_ctx_action(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "action"}
			.parameter("void*", "out_action_data")
			.return_type("void final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_action(ctx, names);
	}
}

auto print_sys_exec_ctx_add(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "add"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("const void*", "component_data")
			.return_type("void final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_add(ctx, names);
	}
}

auto print_sys_exec_ctx_remove(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "remove"}
			.parameter("ecsact_component_like_id", "component_id")
			.return_type("void final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_remove(ctx, names);
	}
}

auto print_sys_exec_ctx_get(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "get"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("void*", "out_component_data")
			.return_type("void final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_get(ctx, names);
	}
}

auto print_sys_exec_ctx_update(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "update"}
			.parameter("ecsact_component_like_id", "component_id")
			.parameter("const void*", "component_data")
			.return_type("void final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_update(ctx, names);
	}
}

auto print_sys_exec_ctx_has(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "has"}
			.parameter("ecsact_component_like_id", "component_id")
			.return_type("bool final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_has(ctx, names);
	}
}

auto print_sys_exec_ctx_generate(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "generate"}
			.parameter("int", "component_count")
			.parameter("ecsact_component_id*", "component_ids")
			.parameter("const void**", "components_data")
			.return_type("void final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_generate(ctx, names);
	}
}

auto print_sys_exec_ctx_parent( //
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "parent"} //
			.return_type("const ecsact_system_execution_context* final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_parent(ctx, names);
	}
}

auto print_sys_exec_ctx_other(
	ecsact::codegen_plugin_context&                                      ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names,
	std::vector<std::shared_ptr<system_provider>> system_providers
) -> void {
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "other"}
			.parameter("ecsact_entity_id", "entity")
			.return_type("ecsact_system_execution_context* final");

	for(const auto& provider : system_providers) {
		auto result = provider->context_function_other(ctx, names);
	}
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
	const ecsact::rt_entt_codegen::core::print_execute_systems_var_names names
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;
	using ecsact::rt_entt_codegen::core::provider::association;
	using ecsact::rt_entt_codegen::core::provider::basic;
	using ecsact::rt_entt_codegen::core::provider::handle_exclusive_provide;
	using ecsact::rt_entt_codegen::core::provider::lazy;
	using ecsact::rt_entt_codegen::core::provider::notify;
	using ecsact::rt_entt_codegen::util::method_printer;

	assert(sys_like_id_variant != system_like_id_variant{});

	auto system_name =
		cpp_identifier(decl_full_name(sys_like_id_variant.get_sys_like_id()));

	auto lazy_provider = std::make_shared<lazy>(sys_like_id_variant);
	auto association_provider =
		std::make_shared<association>(sys_like_id_variant);
	auto notify_provider = std::make_shared<notify>(sys_like_id_variant);
	auto basic_provider = std::make_shared<basic>(sys_like_id_variant);

	std::vector<std::shared_ptr<system_provider>> system_providers{
		lazy_provider,
		association_provider,
		notify_provider,
		basic_provider
	};

	auto sys_details = ecsact_entt_system_details::from_system_like(
		sys_like_id_variant.get_sys_like_id()
	);

	auto sys_caps =
		ecsact::meta::system_capabilities(sys_like_id_variant.get_sys_like_id());

	auto additional_view_components = std::vector<std::string>{};

	for(const auto& provider : system_providers) {
		provider->initialization(ctx, names);
	}

	for(const auto& provider : system_providers) {
		provider->before_make_view_or_group(ctx, names, additional_view_components);
	}

	ecsact::rt_entt_codegen::util::make_view(
		ctx,
		"view",
		names.registry_var_name,
		sys_details,
		additional_view_components
	);

	ctx.write("using view_t = decltype(view);\n");

	for(const auto& provider : system_providers) {
		provider->after_make_view_or_group(ctx, names);
	}

	block(ctx, "struct : ecsact_system_execution_context ", [&] {
		ctx.write("view_t* view;\n");

		for(const auto& provider : system_providers) {
			provider->context_function_header(ctx, names);
		}

		ctx.write("\n");
		print_sys_exec_ctx_action(ctx, names, system_providers);
		print_sys_exec_ctx_add(ctx, names, system_providers);
		print_sys_exec_ctx_remove(ctx, names, system_providers);
		print_sys_exec_ctx_get(ctx, names, system_providers);
		print_sys_exec_ctx_update(ctx, names, system_providers);
		print_sys_exec_ctx_has(ctx, names, system_providers);
		print_sys_exec_ctx_generate(ctx, names, system_providers);
		print_sys_exec_ctx_parent(ctx, names, system_providers);
		print_sys_exec_ctx_other(ctx, names, system_providers);
	});
	ctx.write("context;\n\n");

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

	for(const auto& provider : system_providers) {
		provider->pre_entity_iteration(ctx, names);
	}

	block(ctx, "for(ecsact::entt::entity_id entity : view)", [&] {
		ctx.write("context.entity = entity;\n");

		ecsact::rt_entt_codegen::core::print_child_systems(
			ctx,
			names,
			sys_like_id_variant
		);

		for(const auto& provider : system_providers) {
			provider->pre_exec_system_impl(ctx, names);
		}

		for(const auto& provider : system_providers) {
			auto handle = provider->system_impl(ctx, names);
			if(handle == handle_exclusive_provide::HANDLED) {
				break;
			}
		}

		ctx.write("\n");
	});

	for(const auto& provider : system_providers) {
		provider->post_exec_system_impl(ctx, names);
	}

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
