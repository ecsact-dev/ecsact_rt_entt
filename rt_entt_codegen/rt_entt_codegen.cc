#include <string>
#include <filesystem>
#include <ranges>
#include "core/core.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

#include "rt_entt_codegen/core/core.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "shared/util.hh"

constexpr auto GENERATED_FILE_DISCLAIMER = R"(
// GENERATED FILE - DO NOT EDIT
)";

constexpr auto MAIN_PACKAGE_ONLY_DISCLAIMER = R"(
// Purposely empty. ecsact_rt_entt_codegen is only for the 'main' package
)";

void ecsact_codegen_plugin(
	ecsact_package_id         package_id,
	ecsact_codegen_write_fn_t write_fn
) {
	using ecsact::cc_lang_support::c_identifier;
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::meta::get_all_system_like_ids;
	using ecsact::rt_entt_codegen::ecsact_entt_details;
	using ecsact::rt_entt_codegen::util::global_initializer_printer;
	using ecsact::rt_entt_codegen::util::inc_header;
	using ecsact::rt_entt_codegen::util::inc_package_header;
	using ecsact::rt_entt_codegen::util::init_global;

	ecsact::codegen_plugin_context ctx{package_id, write_fn};

	ctx.write(GENERATED_FILE_DISCLAIMER);

	if(ecsact_meta_main_package() != package_id) {
		ctx.write(MAIN_PACKAGE_ONLY_DISCLAIMER);
		return;
	}

	auto details = ecsact_entt_details::from_package(package_id);

	inc_header(ctx, "ecsact/entt/entity.hh");
	inc_header(ctx, "ecsact/entt/event_markers.hh");
	inc_header(ctx, "ecsact/entt/execution.hh");
	inc_header(ctx, "ecsact/entt/registry_util.hh");
	inc_header(ctx, "ecsact/entt/detail/globals.hh");
	inc_header(ctx, "ecsact/entt/detail/apply_pending.hh");
	inc_header(ctx, "ecsact/entt/wrapper/core.hh");
	inc_header(ctx, "ecsact/entt/wrapper/dynamic.hh");
	inc_header(ctx, "ecsact/entt/error_check.hh");
	ctx.write("\n");
	inc_package_header(ctx, package_id);
	for(auto dep : ecsact::meta::get_dependencies(package_id)) {
		inc_package_header(ctx, dep);
	}
	ctx.write("\n");

	init_global(ctx, "registries");
	init_global(ctx, "last_registry_id");
	init_global(ctx, "system_impls");

	init_global(ctx, "all_component_ids", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write("result.reserve(", details.all_components.size(), ");\n");

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write("result.insert(", cpp_comp_name, "::id);\n");
		}
	});

	init_global(ctx, "add_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write("result.reserve(", details.all_components.size(), ");\n");

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::add_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "get_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		auto non_tag_component_ids =
			details.all_components |
			std::views::filter([&](ecsact_component_id comp_id) -> bool {
				return !ecsact::meta::get_field_ids(comp_id).empty();
			});

		ctx.write(
			"result.reserve(",
			std::ranges::distance(non_tag_component_ids),
			");\n"
		);

		for(auto comp_id : non_tag_component_ids) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::get_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "update_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		auto non_tag_component_ids =
			details.all_components |
			std::views::filter([&](ecsact_component_id comp_id) -> bool {
				return !ecsact::meta::get_field_ids(comp_id).empty();
			});

		ctx.write(
			"result.reserve(",
			std::ranges::distance(non_tag_component_ids),
			");\n"
		);

		for(auto comp_id : non_tag_component_ids) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::update_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "remove_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write("result.reserve(", details.all_components.size(), ");\n");

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::remove_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "has_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write("result.reserve(", details.all_components.size(), ");\n");

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::has_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	ctx.write("\n");

	{ // Print core Ecsact API methods
		using namespace ecsact::rt_entt_codegen;

		core::print_entity_sorting_components(ctx, details);
		core::print_check_error_template_specializations(ctx, details);
		core::print_execute_system_like_template_specializations(ctx, details);
		core::print_init_registry_storage(ctx, details);
		core::print_create_registry(ctx, details);
		core::print_trigger_ecsact_events_minimal(ctx, details);
		core::print_trigger_ecsact_events_all(ctx, details);
		core::print_cleanup_ecsact_component_events(ctx, details);
		core::print_execution_options(ctx, details);
		core::print_execute_systems(ctx, details);
	}
}
