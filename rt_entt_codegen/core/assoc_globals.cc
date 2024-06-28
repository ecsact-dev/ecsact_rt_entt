#include "rt_entt_codegen/core/core.hh"

#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"

using ecsact::cc_lang_support::c_identifier;
using ecsact::meta::decl_full_name;
using ecsact::meta::system_assoc_ids;

auto ecsact::rt_entt_codegen::core::print_assoc_globals(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	for(auto sys_id : details.all_systems) {
		auto assoc_ids = system_assoc_ids(sys_id);
		for(auto i = 0; assoc_ids.size() > i; ++i) {
			auto assoc_id = assoc_ids.at(i);
			auto assoc_global_name =
				std::format("{}__{}", c_identifier(decl_full_name(sys_id)), i);
			ctx.write(std::format(
				"extern \"C\" ecsact_system_assoc_id {} = "
				"static_cast<ecsact_system_assoc_id>({});\n",
				assoc_global_name,
				i
			));
		}
	}
}
