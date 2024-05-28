#include "sys_exec.hh"

#include "rt_entt_codegen/shared/util.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

static auto exec_start_label_name = std::string{};

static auto pending_lazy_exec_struct = std::string{};

static int32_t lazy_iteration_rate{};

auto ecsact::rt_entt_codegen::core::init_lazy(
	ecsact::codegen_plugin_context&                                    ctx,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options options,
	std::vector<std::string>& additional_view_components
) -> void {
	lazy_iteration_rate = 0;

	if(options.is_system()) {
		lazy_iteration_rate = ecsact_meta_get_lazy_iteration_rate(
			static_cast<ecsact_system_id>(options.get_sys_like_id())
		);
	}

	exec_start_label_name =
		std::format("exec_start_{}_", static_cast<int>(options.get_sys_like_id()));

	pending_lazy_exec_struct = std::format(
		"::ecsact::entt::detail::pending_lazy_execution<::{}>",
		options.system_name
	);

	if(lazy_iteration_rate > 0) {
		ctx.write(
			"constexpr auto lazy_iteration_rate_ = ",
			lazy_iteration_rate,
			";\n\n"
		);
		ctx.write("auto iteration_count_ = 0;\n\n");
		ctx.write(exec_start_label_name, ":\n");
		additional_view_components.push_back(pending_lazy_exec_struct);
	}
}

auto ecsact::rt_entt_codegen::core::sort_print_access_lazy(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& sys_details,
	const ecsact::rt_entt_codegen::core::print_execute_systems_options options
) -> void {
	if(lazy_iteration_rate > 0) {
		using ecsact::cpp_codegen_plugin_util::block;

		auto system_sorting_struct_name = std::format(
			"::ecsact::entt::detail::system_sorted<{}>",
			options.system_name
		);

		ctx.write(
			"// If this assertion triggers that's a ecsact_rt_entt codegen "
			"failure\n"
		);
		ctx.write("assert(iteration_count_ <= lazy_iteration_rate_);\n");
		block(ctx, "if(iteration_count_ < lazy_iteration_rate_)", [&] {
			ctx.write(
				"_recalc_sorting_hash<",
				options.system_name,
				">(",
				options.registry_var_name,
				");\n"
			);
			ctx.write(
				options.registry_var_name,
				".sort<",
				system_sorting_struct_name,
				">([](const auto& a, const auto& b) { return a.hash < b.hash; });\n"
			);

			ecsact::rt_entt_codegen::util::make_view(
				ctx,
				"view_no_pending_lazy_",
				options.registry_var_name,
				sys_details
			);

			ctx.write("auto view_no_pending_lazy_count_ = 0;\n");

			block(
				ctx,
				"for(ecsact::entt::entity_id entity : view_no_pending_lazy_)",
				[&] {
					ctx.write(
						"// If this assertion triggers this is an indicator of a codegen "
						"failure.\n"
						"// Please report to "
						"https://github.com/ecsact-dev/ecsact_rt_entt\n"
					);
					ctx.write(
						"assert(",
						options.registry_var_name,
						".all_of<",
						system_sorting_struct_name,
						">(entity));\n"
					);
					ctx.write("view_no_pending_lazy_count_ += 1;\n");
					ctx.write(
						options.registry_var_name,
						".emplace<",
						pending_lazy_exec_struct,
						">(entity);\n"
					);
				}
			);

			block(
				ctx,
				"if(view_no_pending_lazy_count_ >= lazy_iteration_rate_)",
				[&] { ctx.write("goto ", exec_start_label_name, ";\n"); }
			);
		});
	}
}

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

	auto child_system_ids =
		ecsact::meta::get_child_system_ids(options.get_sys_like_id());

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
		for(auto child_sys_id : get_child_system_ids(options.get_sys_like_id())) {
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
