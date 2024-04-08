#include "rt_entt_codegen/core/core.hh"

#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"

using ecsact::cpp_codegen_plugin_util::method_printer;
using ecsact::rt_entt_codegen::ecsact_entt_details;
using ecsact::rt_entt_codegen::util::decl_cpp_ident;

static auto print_system_entity_sorting_component_struct(
	ecsact::codegen_plugin_context& ctx,
	ecsact_system_like_id           sys_like_id
) -> void {
	using ecsact::cpp_codegen_plugin_util::block;

	auto system_cpp_name = decl_cpp_ident(sys_like_id);
	auto caps_map = ecsact::meta::system_capabilities(sys_like_id);

	block(
		ctx,
		std::format(
			"template<> struct ecsact::entt::detail::system_sorted<{}>",
			system_cpp_name
		),
		[&] {
			ctx.write("std::uint64_t hash = 0;\n\n");
			ctx.write(
				"friend auto operator<=>(const system_sorted&, const system_sorted&) = "
				"default;\n\n"
			);

			auto printer = method_printer{ctx, "recalc"};

			auto gen_comp_var_name = [](auto comp_id) -> std::string {
				return std::format("c{}", static_cast<int>(comp_id));
			};

			for(auto&& [comp_id, caps] : caps_map) {
				auto comp_cpp_name = decl_cpp_ident(comp_id);
				auto comp_var = gen_comp_var_name(comp_id);
				printer.parameter(std::format("const {}&", comp_cpp_name), comp_var);
			}

			printer.return_type("void");

			ctx.write("auto bytes = ::ecsact::entt::detail::bytes_copy(");
			ctx.indentation += 1;

			auto prefix = std::string{"\n"};

			for(auto&& [comp_id, caps] : caps_map) {
				auto comp_cpp_name = decl_cpp_ident(comp_id);
				auto comp_var = gen_comp_var_name(comp_id);

				for(auto field_id : ecsact::meta::get_field_ids(comp_id)) {
					auto field_name = std::string{ecsact_meta_field_name(
						ecsact_id_cast<ecsact_composite_id>(comp_id),
						field_id
					)};

					ctx.write(prefix, comp_var, ".", field_name);
					prefix = ",\n";
				}
			}

			ctx.indentation -= 1;
			ctx.write("\n);\n");

			ctx.write(
				"hash = ::ecsact::entt::detail::bytes_hash(bytes.data(), "
				"bytes.size());\n"
			);
		}
	);

	ctx.write(";");

	// block("struct")

	// auto method = method_printer{ctx, ""};
}

static auto system_needs_sorted_entities(
	auto                       id,
	const ecsact_entt_details& details
) -> bool {
	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(id);
	auto needs_sorted_entities = false;

	if(details.is_system(sys_like_id)) {
		auto lazy_rate = ecsact_meta_get_lazy_iteration_rate(
			static_cast<ecsact_system_id>(sys_like_id)
		);

		if(lazy_rate > 0) {
			needs_sorted_entities = true;
		}
	}

	return needs_sorted_entities;
}

auto ecsact::rt_entt_codegen::core::print_entity_sorting_components(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	for(auto sys_id : details.all_systems) {
		if(system_needs_sorted_entities(sys_id, details)) {
			print_system_entity_sorting_component_struct(
				ctx,
				ecsact_id_cast<ecsact_system_like_id>(sys_id)
			);
		}
	}
}
