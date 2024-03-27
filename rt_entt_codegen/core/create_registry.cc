#include "core.hh"

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"

auto ecsact::rt_entt_codegen::core::print_create_registry( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "ecsact_create_registry"}
			.parameter("const char*", "registry_name")
			.return_type("ecsact_registry_id");

	ctx.write("auto&& [registry_id, reg] = ecsact::entt::create_registry();\n\n");

	if(!details.group_systems.empty()) {
		ctx.write(
			"// These groups were automatically selected based on the input ecsact "
			"files\n"
		);
		for(auto sys_id : details.group_systems) {
			auto decl_name = ecsact::meta::decl_full_name(sys_id);
		}
	}

	ctx.write("ecsact_init_registry_storage(registry_id);\n");

	ctx.write("\nreturn registry_id;");
}
