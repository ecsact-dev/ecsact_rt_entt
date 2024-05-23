#include "parallel.hh"

#include <set>
#include <vector>
#include <type_traits>

#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

static auto loop_iterator(
	const std::vector<ecsact_system_like_id>&          system_list,
	const std::vector<ecsact_system_like_id>::iterator begin,
	std::vector<std::vector<ecsact_system_like_id>>&   parallel_system_cluster
) -> void;

auto ecsact::rt_entt_codegen::parallel::get_parallel_execution_cluster(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details,
	std::vector<ecsact_system_like_id>                  system_list,
	std::string                                         parent_context
) -> std::vector<std::vector<ecsact_system_like_id>> {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;

	auto parallel_system_cluster =
		std::vector<std::vector<ecsact_system_like_id>>{};

	loop_iterator(system_list, system_list.begin(), parallel_system_cluster);

	return parallel_system_cluster;
}

static auto is_capability_safe(ecsact_system_capability capability) -> bool {
	std::underlying_type_t<ecsact_system_capability> unsafe_caps =
		ECSACT_SYS_CAP_ADDS | ECSACT_SYS_CAP_REMOVES | ECSACT_SYS_CAP_WRITEONLY;
	unsafe_caps &= ~(ECSACT_SYS_CAP_EXCLUDE | ECSACT_SYS_CAP_INCLUDE);

	return (unsafe_caps & capability) == 0b0;
}

static auto loop_iterator(
	const std::vector<ecsact_system_like_id>&          system_list,
	const std::vector<ecsact_system_like_id>::iterator begin,
	std::vector<std::vector<ecsact_system_like_id>>&   parallel_system_cluster
) -> void {
	std::vector<ecsact_system_like_id> parallel_system_list;
	auto unsafe_comps = std::set<ecsact_component_like_id>{};

	using ecsact::meta::decl_full_name;

	for(auto iterator = begin; iterator != system_list.end(); iterator++) {
		auto sys_like_id = *iterator;
		auto capabilities = ecsact::meta::system_capabilities(sys_like_id);

		auto generate_ids = ecsact::meta::get_system_generates_ids(sys_like_id);

		if(!generate_ids.empty()) {
			if(!parallel_system_list.empty()) {
				parallel_system_cluster.push_back(parallel_system_list);
			}

			parallel_system_cluster.push_back(
				std::vector<ecsact_system_like_id>{sys_like_id}
			);
			loop_iterator(
				system_list,
				std::next(iterator, 1),
				parallel_system_cluster
			);
			return;
		}

		std::set<ecsact_component_like_id> child_unsafe_comps{};

		auto child_systems = ecsact::meta::get_child_system_ids(sys_like_id);

		for(auto child_sys_id : child_systems) {
			auto cpp_system_name = decl_full_name(child_sys_id);
			auto child_capabilities = ecsact::meta::system_capabilities(child_sys_id);
			for(auto const [child_comp_id, child_capability] : child_capabilities) {
				if(unsafe_comps.contains(child_comp_id)) {
					if(child_capability == ECSACT_SYS_CAP_READONLY ||
						 child_capability == ECSACT_SYS_CAP_OPTIONAL_READONLY) {
						parallel_system_cluster.push_back(parallel_system_list);
						loop_iterator(system_list, iterator, parallel_system_cluster);
						return;
					}
				}

				if(!is_capability_safe(child_capability)) {
					if(unsafe_comps.contains(child_comp_id)) {
						parallel_system_cluster.push_back(parallel_system_list);
						loop_iterator(system_list, iterator, parallel_system_cluster);
						return;
					} else {
						child_unsafe_comps.insert(child_comp_id);
					}
				}
			}
		}

		for(const auto [comp_id, capability] : capabilities) {
			auto cpp_name = decl_full_name(comp_id);

			if(unsafe_comps.contains(comp_id)) {
				if(capability == ECSACT_SYS_CAP_READONLY ||
					 capability == ECSACT_SYS_CAP_OPTIONAL_READONLY) {
					parallel_system_cluster.push_back(parallel_system_list);
					loop_iterator(system_list, iterator, parallel_system_cluster);
					return;
				}
			}

			if(!is_capability_safe(capability)) {
				if(!unsafe_comps.contains(comp_id)) {
					unsafe_comps.insert(comp_id);
				} else {
					parallel_system_cluster.push_back(parallel_system_list);
					loop_iterator(system_list, iterator, parallel_system_cluster);
					return;
				}
			}

			auto other_fields =
				ecsact::meta::system_association_fields(sys_like_id, comp_id);

			for(auto field_id : other_fields) {
				auto other_capabilities = ecsact::meta::system_association_capabilities(
					sys_like_id,
					comp_id,
					field_id
				);

				for(const auto [other_comp_id, other_capability] : other_capabilities) {
					auto cpp_name = decl_full_name(other_comp_id);
					if(!is_capability_safe(other_capability)) {
						if(!unsafe_comps.contains(other_comp_id)) {
							unsafe_comps.insert(other_comp_id);
						} else {
							parallel_system_cluster.push_back(parallel_system_list);
							loop_iterator(system_list, iterator, parallel_system_cluster);
							return;
						}
					}
				}
			}
		}

		for(auto unsafe_comp : child_unsafe_comps) {
			if(!unsafe_comps.contains(unsafe_comp)) {
				unsafe_comps.insert(unsafe_comp);
			}
		}

		parallel_system_list.push_back(sys_like_id);
	}
	parallel_system_cluster.push_back(parallel_system_list);
}
