
#include "rt_entt_codegen/core/core.hh"

#include <format>
#include <ranges>
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using ecsact::cc_lang_support::c_identifier;
using ecsact::cc_lang_support::cpp_field_type_name;
using ecsact::cc_lang_support::cpp_identifier;
using ecsact::cpp_codegen_plugin_util::block;
using ecsact::cpp_codegen_plugin_util::comma_delim;
using ecsact::cpp_codegen_plugin_util::method_printer;
using ecsact::meta::decl_full_name;
using ecsact::meta::system_assoc_ids;

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

auto ecsact::rt_entt_codegen::core::print_assoc_fields_hash(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	auto printer = //
		method_printer{ctx, "ecsact::entt::detail::get_assoc_fields_hash"} //
			.parameter("ecsact_composite_id", "compo_id")
			.parameter("std::va_list", "indexed_field_values")
			.return_type("std::uint64_t");

	for(auto comp_id : details.all_components) {
		auto compo_cpp_identifier = cpp_identifier(decl_full_name(comp_id));
		auto assoc_fields = get_assoc_fields(comp_id);
		if(assoc_fields.empty()) {
			continue;
		}

		block(
			ctx,
			std::format(
				"if(ecsact_id_cast<ecsact_composite_id>({}::id) == compo_id)",
				compo_cpp_identifier
			),
			[&] {
				for(auto field_id : assoc_fields) {
					auto field_name = ecsact::meta::field_name(comp_id, field_id);
					auto field_type = ecsact::meta::get_field_type(comp_id, field_id);
					auto field_cpp_type = cpp_field_type_name(field_type);
					ctx.write(std::format(
						"auto {} = va_arg(indexed_field_values, {});\n",
						field_name,
						field_cpp_type
					));
				}

				ctx.write(std::format(
					"return "
					"::ecsact::entt::detail::hash_vals(static_cast<int32_t>({}::id), "
					"{});",
					compo_cpp_identifier,
					comma_delim(std::views::transform(
						assoc_fields,
						[&](auto field_id) -> std::string {
							return ecsact::meta::field_name(comp_id, field_id);
						}
					))
				));
			}
		);
		ctx.write("\n");
	}

	ctx.write("return 0;");
}
