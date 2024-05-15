#pragma once

#include "ecsact/runtime/meta.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen::system_util {

namespace detail {
/*
 * Checks if a system uses notify and should implement the run_system<S>
 * component in its execution
 */
auto is_notify_system(ecsact_system_like_id system_id) -> bool;

/*
 * Prints the specialized views for ecsact_system_notify_settings components
 */
auto print_system_notify_views(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	ecsact_system_like_id                                      system_id,
	std::string                                                registry_name
) -> void;
} // namespace detail

template<typename SystemLikeID>
auto is_notify_system(SystemLikeID system_id) -> bool {
	return detail::is_notify_system(
		ecsact_id_cast<ecsact_system_like_id>(system_id)
	);
}

template<typename SystemLikeID>
auto print_system_notify_views(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	SystemLikeID                                               system_id,
	std::string                                                registry_name
) -> void {
	detail::print_system_notify_views(
		ctx,
		details,
		ecsact_id_cast<ecsact_system_like_id>(system_id),
		registry_name
	);
}

auto is_trivial_system(ecsact_system_like_id system_id) -> bool;

auto get_unique_view_name() -> std::string;

template<typename ComponentLikeID>
static auto create_context_struct_name( //
	ComponentLikeID component_like_id
) -> std::string {
	using ecsact::cc_lang_support::c_identifier;
	auto full_name =
		c_identifier(ecsact::meta::decl_full_name(component_like_id));
	return full_name + "Struct";
}

template<typename ComponentLikeID>
static auto create_context_var_name( //
	ComponentLikeID component_like_id
) -> std::string {
	using ecsact::cc_lang_support::c_identifier;
	auto full_name =
		c_identifier(ecsact::meta::decl_full_name(component_like_id));
	return full_name + "_context";
}

} // namespace ecsact::rt_entt_codegen::system_util
