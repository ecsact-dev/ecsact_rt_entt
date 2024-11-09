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

	ctx.write("auto& entt_reg = ecsact::entt::get_registry(registry_id);\n\n");
	ctx.write("ecsact_init_registry_storage(entt_reg);\n");
	ctx.write("\nreturn registry_id;");
}
