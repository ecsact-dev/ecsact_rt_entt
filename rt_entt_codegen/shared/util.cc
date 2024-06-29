#include "util.hh"

#include <map>

using namespace std::string_literals;
using ecsact::cc_lang_support::cpp_identifier;
using ecsact::meta::decl_full_name;
using std::views::transform;

auto ecsact::rt_entt_codegen::util::make_view( //
	ecsact::codegen_plugin_context& ctx,
	make_view_options               opts
) -> void {
	ctx.write(
		"auto ",
		opts.view_var_name,
		" = ",
		opts.registry_var_name,
		".view<"
	);

	// components that may have multiple instances
	auto multi_components = std::vector<ecsact_component_like_id>{};
	auto is_multi_component = [&](auto id) {
		auto itr = std::ranges::find( //
			multi_components,
			ecsact_id_cast<ecsact_component_like_id>(id)
		);
		return itr != multi_components.end();
	};

	if(!opts.without_multi_component_storage) {
		for(auto assoc_info : opts.details.association_details) {
			if(!is_multi_component(assoc_info.component_id)) {
				multi_components.push_back(assoc_info.component_id);
			}
		}
	}

	auto exclude_multi_components = [&] {
		return std::views::filter([&](auto id) -> bool {
			return !is_multi_component(id);
		});
	};

	ctx.write(comma_delim(
		opts.details.get_comps | exclude_multi_components() |
		transform(decl_cpp_ident<ecsact_component_like_id>)
	));

	if(!multi_components.empty() &&
		 !(opts.details.get_comps | exclude_multi_components()).empty()) {
		ctx.write(", ");
	}

	ctx.write(comma_delim(
		multi_components | //
		transform([](auto id) -> std::string {
			return std::format(
				"ecsact::entt::detail::multi_assoc_storage<{}>",
				decl_cpp_ident(id)
			);
		})
	));

	for(auto comp_id : opts.details.writable_comps | exclude_multi_components()) {
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
				"::ecsact::entt::detail::hash_vals32(static_cast<int32_t>({}::id), "
				"{})",
				compo_name,
				comma_delim(std::views::transform(
					fields,
					[](auto& entry) -> std::string { return entry.second; }
				))
			);
			ctx.write(std::format(
				"view.storage({}.storage<::ecsact::entt::indexed<{}>>({}));\n",
				opts.registry_var_name,
				compo_name,
				hash_fields_str
			));
		}
	}
}
