#include "ecsact/codegen/plugin.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen::parallel {
auto get_parallel_execution_cluster(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details,
	std::vector<ecsact_system_like_id>                  system_list,
	std::string                                         parent_context = "nullptr"
) -> std::vector<std::vector<ecsact_system_like_id>>;
}
