
#include "core.hh"

#include <format>
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/common.h"
#include "ecsact/cpp_codegen_plugin_util.hh"

using ecsact::codegen_plugin_context;
using ecsact::cc_lang_support::cpp_identifier;
using ecsact::cpp_codegen_plugin_util::method_printer;
using ecsact::meta::decl_full_name;
using ecsact::rt_entt_codegen::ecsact_entt_details;

static auto print_update_indexed_storage_component(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details,
	ecsact_component_like_id   comp_like_id
) -> void {
	auto comp_cpp_ident = cpp_identifier(decl_full_name(comp_like_id));

	ctx.write("template<> ");
	auto printer =
		method_printer{
			ctx,
			std::format(
				"ecsact::entt::detail::update_indexed_storage<{}>",
				comp_cpp_ident
			)
		}
			.parameter("ecsact::entt::registry_t&", "reg")
			.return_type("void");
}

auto ecsact::rt_entt_codegen::core::print_update_indexed_storage(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	for(auto comp_id : details.all_components) {
		print_update_indexed_storage_component(
			ctx,
			details,
			ecsact_id_cast<ecsact_component_like_id>(comp_id)
		);
	}
}
