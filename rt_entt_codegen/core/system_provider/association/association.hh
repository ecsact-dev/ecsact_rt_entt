#pragma once

#include <string>
#include <map>
#include <set>
#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/core/system_provider/system_ctx_functions.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class association final : public system_provider {
public:
	using system_provider::system_provider;

	auto initialization( //
		codegen_plugin_context& ctx,
		const common_vars&      names
	) -> void final;

	auto entity_iteration(
		codegen_plugin_context& ctx,
		const common_vars&      names,
		std::function<void()>   iter_func
	) -> handle_exclusive_provide final;

	auto after_make_view_or_group(
		codegen_plugin_context& ctx,
		const common_vars&      names
	) -> void final;

	auto context_function_header(
		codegen_plugin_context& ctx,
		const common_vars&      names
	) -> void final;

	auto context_function_other(
		codegen_plugin_context& ctx,
		const common_vars&      names
	) -> handle_exclusive_provide final;

	auto pre_entity_iteration(
		codegen_plugin_context& ctx,
		const common_vars&      names
	) -> void final;

	auto pre_exec_system_impl(
		codegen_plugin_context& ctx,
		const common_vars&      names
	) -> void final;

	auto system_impl( //
		codegen_plugin_context& ctx,
		const common_vars&      names
	) -> handle_exclusive_provide final;

private:
	std::map<ecsact_system_assoc_id, std::string>               assoc_view_names;
	std::map<ecsact_composite_id, std::vector<ecsact_field_id>> assoc_fields;

	// List of composites the association needs to read from in their view/group
	// due to indexed fields.
	std::map<ecsact_system_assoc_id, std::set<ecsact_composite_id>>
		assoc_composites;

	auto print_other_contexts(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> void;

	auto print_other_ctx_action(codegen_plugin_context& ctx) -> void;
	auto print_other_ctx_add(
		codegen_plugin_context&           ctx,
		const capability_t&               other_caps,
		const ecsact_entt_system_details& details
	) -> void;

	auto print_other_ctx_remove(
		codegen_plugin_context&           ctx,
		const capability_t&               other_caps,
		const ecsact_entt_system_details& details,
		const std::string&                view_type_name
	) -> void;

	auto print_other_ctx_get(
		codegen_plugin_context&           ctx,
		const ecsact_entt_system_details& details,
		const std::string&                view_type_name
	) -> void;

	auto print_other_ctx_update(
		codegen_plugin_context&           ctx,
		const ecsact_entt_system_details& details,
		const std::string&                view_type_name
	) -> void;

	auto print_other_ctx_has(
		codegen_plugin_context&           ctx,
		const ecsact_entt_system_details& details
	) -> void;

	auto print_other_ctx_generate(
		codegen_plugin_context&           ctx,
		const ecsact_entt_system_details& details
	) -> void;

	auto print_other_ctx_parent(
		codegen_plugin_context&       ctx,
		const system_like_id_variant& sys_like_id
	) -> void;

	auto print_other_ctx_other(
		codegen_plugin_context&           ctx,
		const ecsact_entt_system_details& details
	) -> void;
};
} // namespace ecsact::rt_entt_codegen::core::provider
