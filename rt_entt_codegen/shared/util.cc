#include "util.hh"

#include <map>

auto ecsact::rt_entt_codegen::util::make_view( //
	ecsact::codegen_plugin_context& ctx,
	make_view_options               opts
) -> void {
	using namespace std::string_literals;
	using ecsact::rt_entt_codegen::util::comma_delim;
	using ecsact::rt_entt_codegen::util::decl_cpp_ident;
	using std::views::transform;

	ctx.write(
		"auto ",
		opts.view_var_name,
		" = ",
		opts.registry_var_name,
		".view<"
	);

	ctx.write(comma_delim(
		opts.details.get_comps | transform(decl_cpp_ident<ecsact_component_like_id>)
	));

	for(auto comp_id : opts.details.writable_comps) {
		auto comp_name = decl_cpp_ident(comp_id);

		opts.additional_components.push_back(std::format(
			"ecsact::entt::detail::exec_beforechange_storage<{}>",
			comp_name
		));
	}

	if(!opts.additional_components.empty()) {
		ctx.write(", ");
		ctx.write(comma_delim(opts.additional_components));
	}

	ctx.write(">(");

	auto exclude_comps = opts.details.exclude_comps |
		transform(decl_cpp_ident<ecsact_component_like_id>);

	opts.additional_exclude_components.insert(
		opts.additional_exclude_components.end(),
		exclude_comps.begin(),
		exclude_comps.end()
	);

	if(!opts.additional_exclude_components.empty()) {
		ctx.write(
			"::entt::exclude<",
			comma_delim(opts.additional_exclude_components),
			">"
		);
	}

	ctx.write(");\n");

	if(opts.sys_like_id && opts.sys_like_id->is_action()) {
		auto act_id = opts.sys_like_id->as_action();
		auto indexed_fields = std::map<
			ecsact_composite_id,
			std::vector<std::pair<ecsact_field_id, std::string>>>{};
		for(auto field_id : ecsact::meta::get_field_ids(act_id)) {
			auto act_field_name = ecsact::meta::field_name(act_id, field_id);
			auto field_type = ecsact::meta::get_field_type(act_id, field_id);
			if(field_type.kind != ECSACT_TYPE_KIND_FIELD_INDEX) {
				continue;
			}

			indexed_fields[field_type.type.field_index.composite_id].push_back({
				field_type.type.field_index.field_id,
				std::format("action.{}", act_field_name),
			});
		}

		for(auto&& [compo_id, fields] : indexed_fields) {
			auto compo_name = cpp_identifier(decl_full_name(compo_id));
			auto hash_fields_str = std::format(
				"::ecsact::entt::detail::hash_vals(static_cast<int32_t>({}::id), {})",
				compo_name,
				comma_delim(std::views::transform(
					fields,
					[](auto& entry) -> std::string { return entry.second; }
				))
			);
			ctx.write(std::format(
				"view.storage({}.storage<::ecsact::entt::indexed<{}>>({}));\n",
				names.registry_var_name,
				compo_name,
				hash_fields_str
			));
		}
	}
}
