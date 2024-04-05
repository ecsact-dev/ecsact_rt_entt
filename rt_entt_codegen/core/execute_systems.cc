#include "core.hh"

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

constexpr auto METHOD_BODY_TOP = R"(
auto& reg = ecsact::entt::get_registry(registry_id);
auto actions = ecsact::entt::actions_map{};
)";

auto ecsact::rt_entt_codegen::core::print_execute_systems( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "ecsact_execute_systems"}
			.parameter("ecsact_registry_id", "registry_id")
			.parameter("int", "execution_count")
			.parameter("const ecsact_execution_options*", "execution_options_list")
			.parameter("const ecsact_execution_events_collector*", "evc")
			.return_type("ecsact_execute_systems_error");

	ctx.write(METHOD_BODY_TOP);

	ctx.write("\n");

	ctx.write("for(auto i=0; execution_count > i; ++i) {");
	ctx.indentation += 1;
	ctx.write("\n");

	ctx.write("actions.collect(i, execution_count, execution_options_list);\n");

	block(ctx, "if(execution_options_list != nullptr)", [&] {
		ctx.write(
			"auto err = handle_execution_options(registry_id, "
			"execution_options_list[i]);\n\n"
		);
		block(ctx, "if(err != ECSACT_EXEC_SYS_OK)", [&] {
			ctx.write("return err;");
		});
	});

	for(auto sys_like : details.top_execution_order) {
		auto cpp_decl_name = cpp_identifier(ecsact::meta::decl_full_name(sys_like));

		if(details.is_action(sys_like)) {
			ctx.write(
				"ecsact::entt::execute_actions<",
				cpp_decl_name,
				">(reg, actions.as_action_span<",
				cpp_decl_name,
				">());\n"
			);
		} else if(details.is_system(sys_like)) {
			ctx.write(
				"ecsact::entt::execute_system<",
				cpp_decl_name,
				">(reg, nullptr);\n"
			);
		} else {
			ctx.write("// ??? unhandled ??? ", cpp_decl_name, "\n");
		}
	}

	ctx.indentation -= 1;
	ctx.write("\n}\n\n");

	block(ctx, "if(evc != nullptr)", [&] {
		ctx.write(
			"auto events_collector = "
			"ecsact::entt::detail::execution_events_collector{};\n"
		);
		ctx.write("events_collector.target = evc;\n\n");

		block(ctx, "if(execution_options_list == nullptr)", [&] {
			ctx.write(
				"trigger_component_events_minimal(registry_id, events_collector);\n\n"
			);
		});

		block(ctx, "else", [&] {
			ctx.write(
				"trigger_component_events_all(registry_id, events_collector);\n\n"
			);
		});
		ctx.write("cleanup_component_events(registry_id);\n");
	});

	ctx.write("return ECSACT_EXEC_SYS_OK;");
}
