#include "sys_exec.hh"

#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/runtime/meta.hh"
#include "rt_entt_codegen/shared/parallel.hh"

auto ecsact::rt_entt_codegen::core::print_child_systems(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details&        details,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& sys_details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options options
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::meta::get_child_system_ids;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;

	auto child_system_ids = ecsact::meta::get_child_system_ids(
		options.sys_like_id_variant.get_sys_like_id()
	);

	std::vector<ecsact_system_like_id> child_system_like_ids{};

	child_system_like_ids.resize(child_system_ids.size());

	std::transform(
		child_system_ids.begin(),
		child_system_ids.end(),
		child_system_like_ids.begin(),
		[](auto system_id) {
			return ecsact_id_cast<ecsact_system_like_id>(system_id);
		}
	);

	if(child_system_ids.size() == 1) {
		// TODO(Kelwan): Make use case system agnostic when we support
		// nested Action systems
		// Issue: https://github.com/ecsact-dev/ecsact_parse/issues/154
		for(auto child_sys_id :
				get_child_system_ids(options.sys_like_id_variant.get_sys_like_id())) {
			auto child_details = ecsact_entt_system_details::from_system_like(
				ecsact_id_cast<ecsact_system_like_id>(child_sys_id)
			);

			auto child_system_name = cpp_identifier(decl_full_name(child_sys_id));

			ctx.write(
				"ecsact::entt::execute_system<::" + child_system_name + ">(",
				options.registry_var_name,
				", &context, {});\n"
			);
		}
	} else {
		auto parallel_system_cluster =
			ecsact::rt_entt_codegen::parallel::get_parallel_execution_cluster(
				ctx,
				details,
				child_system_like_ids
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
						"parent_context"
					));
				}
				if(details.is_system(sync_sys_id)) {
					ctx.write(std::format(
						"ecsact::entt::execute_system<{}>(registry, {}, "
						"actions_map);\n",
						sync_sys_name,
						"parent_context"
					));
				}
				continue;
			}

			ctx.write("execute_parallel_cluster(registry, parent_context, ");
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
	}

	ctx.write("\n");
}
