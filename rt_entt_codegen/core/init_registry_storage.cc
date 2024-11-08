#include "core.hh"

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"

auto ecsact::rt_entt_codegen::core::print_init_registry_storage(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "ecsact_init_registry_storage"}
			.parameter("ecsact_registry_id", "registry_id")
			.return_type("void");

	ctx.write("auto& reg = ecsact::entt::get_registry(registry_id);\n");
	ctx.write(
		"reg.template storage<ecsact::entt::detail::destroyed_entity>();\n\n"
	);

	for(auto comp_id : details.all_components) {
		auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));

		ctx.write(std::format(
			"ecsact::entt::wrapper::core::prepare_component<{}>(registry_id);\n",
			cpp_comp_name
		));
	}

	for(auto system_id : details.all_systems) {
		auto cpp_sys_name = cpp_identifier(decl_full_name(system_id));

		ctx.write(std::format(
			"ecsact::entt::wrapper::core::prepare_system<{}>(registry_id);\n",
			cpp_sys_name
		));
	}
}
