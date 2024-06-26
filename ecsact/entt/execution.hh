#pragma once

#include <vector>
#include <unordered_map>
#include <cassert>
#include <span>
#include "ecsact/runtime/common.h"
#include "ecsact/entt/entity.hh"
#include "entt/entity/registry.hpp"
#include "ecsact/entt/detail/globals.hh"

namespace ecsact::entt::detail {
template<typename>
constexpr bool unimplemented_by_codegen = false;
}

namespace ecsact::entt {

template<typename S>
auto get_system_impl() -> ecsact_system_execution_impl {
	using ecsact::entt::detail::globals::system_impls;

	const auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(S::id);

	auto sys_impl_itr = system_impls.find(sys_like_id);
	if(sys_impl_itr != system_impls.end()) {
		return sys_impl_itr->second;
	} else {
		return nullptr;
	}
}

struct actions_map {
	using raw_value_t =
		std::unordered_map<ecsact_action_id, std::vector<const void*>>;

	raw_value_t raw_value = {};

	/**
	 *  Collect from execution options passed to `ecsact_execute_systems`
	 */
	inline auto collect( //
		int32_t                         index,
		int32_t                         execution_count,
		const ecsact_execution_options* execution_options_list
	) -> void {
		raw_value.clear();

		if(execution_options_list == nullptr) {
			return;
		}

		assert(index < execution_count);
		auto options = execution_options_list[index];

		if(options.actions_length == 0) {
			return;
		}

		auto actions_span = std::span{
			options.actions,
			static_cast<size_t>(options.actions_length),
		};

		for(auto& action : actions_span) {
			raw_value[action.action_id].push_back(action.action_data);
		}
	}

	template<typename Action>
	auto as_action_span() const -> const std::span<const Action* const> {
		ecsact_action_id action_id = Action::id;

		if(!raw_value.contains(action_id)) {
			return {};
		}

		auto& action_data_list = raw_value.at(action_id);
		auto  action_list_data_ptr = action_data_list.data();

		return std::span<const Action* const>{
			reinterpret_cast<const Action* const*>(action_list_data_ptr),
			action_data_list.size(),
		};
	}
};

/**
 * Executes system implementation for @tp System.
 *
 * NOTE: Template specializations are made available via the codegen plugin
 *       `ecsact_rt_entt_codegen`.
 */
template<typename System>
auto execute_system( //
	ecsact::entt::registry_t&        registry,
	ecsact_system_execution_context* parent,
	const ecsact::entt::actions_map& actions_map
) -> void {
	static_assert(detail::unimplemented_by_codegen<System>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR                                                           |
| `execute_system<>` template specialization cannot be found. This is         |
| typically generated by ecsact_rt_entt_codegen.                              |
 -----------------------------------------------------------------------------
)");
}

/**
 * Executes action implementation for @tp Action for every action in @p
 * action_range.
 *
 * NOTE: Template specializations are made available via the codegen plugin
 *       `ecsact_rt_entt_codegen`.
 */
template<typename Action>
auto execute_actions( //
	ecsact::entt::registry_t&        registry,
	ecsact_system_execution_context* parent,
	const ecsact::entt::actions_map& actions_map
) -> void {
	static_assert(detail::unimplemented_by_codegen<Action>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR                                                           |
| `execute_actions<>` template specialization cannot be found. This is        |
| typically generated by ecsact_rt_entt_codegen.                              |
 -----------------------------------------------------------------------------
)");
}

using execute_fn_t = void (*)(
	ecsact::entt::registry_t&        registry,
	ecsact_system_execution_context* parent,
	const ecsact::entt::actions_map& actions_map
);

/**
 * Allocates EnTT groups and storage if necessary for the system or action.
 * NOTE: Template specializations are made available via the codegen plugin
 *       `ecsact_rt_entt_codegen`.
 */
template<typename SystemLike>
auto prepare_system_like(ecsact::entt::registry_t&) -> void {
	static_assert(detail::unimplemented_by_codegen<SystemLike>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR                                                           |
| `prepare_system_like<>` template specialization cannot be found. This is    |
| typically generated by ecsact_rt_entt_codegen.                              |
 -----------------------------------------------------------------------------
)");
}

/**
 * Checks entity 'matches' a system i.e. the system will execute on the provided
 * entity.
 */
template<typename SystemLike>
auto entity_matches_system(ecsact::entt::registry_t&, ecsact::entt::entity_id)
	-> bool {
	static_assert(detail::unimplemented_by_codegen<SystemLike>, R"(
 -----------------------------------------------------------------------------
| (!) CODEGEN ERROR                                                           |
| `entity_matches_system<>` template specialization cannot be found. This is  |
| typically generated by ecsact_rt_entt_codegen.                              |
 -----------------------------------------------------------------------------
)");
}

} // namespace ecsact::entt
