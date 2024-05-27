#include "core.hh"

#include "rt_entt_codegen/shared/parallel.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

constexpr auto METHOD_BODY_TOP = R"(
auto& registry = ecsact::entt::get_registry(registry_id);
auto actions_map = ecsact::entt::actions_map{};
)";

auto ecsact::rt_entt_codegen::core::print_parallel_system_execute(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::rt_entt_codegen::util::method_printer;

	ctx.write("template<std::size_t N>\n");
	auto printer = //
		method_printer{ctx, "execute_parallel_cluster"} //
			.parameter("::entt::registry&", "registry")
			.parameter("ecsact_system_execution_context*", "parent_context")
			.parameter("std::array<exec_entry_t, N>", "system_arr")
			.return_type("void");

	block(
		ctx,
		"std::for_each(std::execution::par_unseq, system_arr.begin(), "
		"system_arr.end(), [&registry](exec_entry_t pair)",
		[&]() {
			ctx.write("auto fn_ptr = pair.first;\n");
			ctx.write("auto& actions_map = pair.second;\n");
			ctx.write("fn_ptr(registry, nullptr, actions_map);");
		}
	);
	ctx.write(");\n");
}

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

	ctx.write( //
		"actions_map.collect(i, execution_count, execution_options_list);\n"
	);

	block(ctx, "if(execution_options_list != nullptr)", [&] {
		ctx.write(
			"auto err = handle_execution_options(registry_id, "
			"execution_options_list[i]);\n\n"
		);
		block(ctx, "if(err != ECSACT_EXEC_SYS_OK)", [&] {
			ctx.write("return err;");
		});
	});

	auto parallel_system_cluster =
		ecsact::rt_entt_codegen::parallel::get_parallel_execution_cluster(
			ctx,
			details,
			details.top_execution_order
		);

	for(const auto& systems_to_parallel : parallel_system_cluster) {
		if(systems_to_parallel.size() == 1) {
			auto sync_sys_id = systems_to_parallel[0];

			auto sync_sys_name =
				cpp_identifier(ecsact::meta::decl_full_name(sync_sys_id));

			if(details.is_action(sync_sys_id)) {
				ctx.write(std::format(
					"ecsact::entt::execute_actions<{}>(registry, {}, "
					"actions_map);\n",
					sync_sys_name,
					"nullptr"
				));
			}
			if(details.is_system(sync_sys_id)) {
				ctx.write(std::format(
					"ecsact::entt::execute_system<{}>(registry, {}, "
					"actions_map);\n",
					sync_sys_name,
					"nullptr"
				));
			}
			continue;
		}

		ctx.write("execute_parallel_cluster(registry, nullptr, ");
		ctx.write(std::format(
			"std::array<exec_entry_t, {}> {{\n",
			systems_to_parallel.size()
		));
		for(const auto system_like_id : systems_to_parallel) {
			auto cpp_decl_name =
				cpp_identifier(ecsact::meta::decl_full_name(system_like_id));

			if(details.is_action(system_like_id)) {
				ctx.write(
					"\texec_entry_t{&ecsact::entt::execute_actions<",
					cpp_decl_name,
					">, actions_map},\n"
				);
			} else if(details.is_system(system_like_id)) {
				ctx.write(
					"\texec_entry_t{&ecsact::entt::execute_system<",
					cpp_decl_name,
					">, actions_map},\n"
				);
			} else {
				ctx.write("// ??? unhandled ??? ", cpp_decl_name, "\n");
			}
		}
		ctx.write("});\n");
	}

	ctx.write("\nupdate_all_beforechange_storage(registry_id);\n");
	ctx.write("cleanup_system_notifies(registry_id);\n");

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
	});

	ctx.write("cleanup_component_events(registry_id);\n");

	ctx.write("return ECSACT_EXEC_SYS_OK;");
}
