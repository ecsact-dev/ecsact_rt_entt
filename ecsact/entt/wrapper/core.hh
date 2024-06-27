#pragma once

#include <cassert>
#include <type_traits>
#include "ecsact/runtime/common.h"
#include "ecsact/entt/detail/internal_markers.hh"
#include "ecsact/entt/event_markers.hh"
#include "entt/entity/registry.hpp"
#include "ecsact/entt/registry_util.hh"
#include "ecsact/entt/error_check.hh"
#include "ecsact/entt/detail/execution_events_collector.hh"
#include "ecsact/entt/detail/assoc_fields_hash.hh"

namespace ecsact::entt::wrapper::core {

template<typename C>
inline auto has_component( //
	ecsact_registry_id                       registry_id,
	ecsact_entity_id                         entity_id,
	[[maybe_unused]] ecsact_component_id     component_id,
	ecsact::entt::detail::assoc_hash_value_t assoc_fields_hash
) -> bool {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);
	if constexpr(C::has_assoc_fields) {
		auto storage_id = static_cast<::entt::id_type>(assoc_fields_hash);
		return reg.storage<C>(storage_id).contains(entity);
	} else {
		return reg.storage<C>().contains(entity);
	}
}

template<typename C>
inline auto get_component(
	ecsact_registry_id                       registry_id,
	ecsact_entity_id                         entity_id,
	[[maybe_unused]] ecsact_component_id     component_id,
	ecsact::entt::detail::assoc_hash_value_t assoc_fields_hash
) -> const void* {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);
#ifndef NDEBUG
	if(C::id != component_id) {
		return nullptr;
	}
	if(!has_component<C>(
			 registry_id,
			 entity_id,
			 component_id,
			 assoc_fields_hash
		 )) {
		return nullptr;
	}
#endif

	if constexpr(C::has_assoc_fields) {
		const C& comp =
			reg.storage<C>(static_cast<::entt::id_type>(assoc_fields_hash))
				.get(entity);
		return &comp;
	} else {
		const C& comp = reg.get<C>(entity);
		return &comp;
	}
}

template<typename C>
inline auto add_component( //
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id,
	const void*                          component_data
) -> ecsact_add_error {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	auto err = ecsact::entt::check_add_component_error<C>(
		reg,
		entity,
		*static_cast<const C*>(component_data)
	);

	if(err == ECSACT_ADD_OK) {
		if constexpr(std::is_empty_v<C>) {
			reg.emplace<C>(entity);
		} else if constexpr(C::has_assoc_fields) {
			auto comp = static_cast<const C*>(component_data);
			auto assoc_fields_hash =
				ecsact::entt::detail::get_assoc_fields_hash(*comp);
			auto storage_id = static_cast<::entt::id_type>(assoc_fields_hash);
			reg.storage<C>(storage_id).emplace(entity, *comp);
		} else {
			auto comp = static_cast<const C*>(component_data);
			reg.emplace<detail::exec_beforechange_storage<C>>(entity, *comp, false);
			reg.emplace<C>(entity, *comp);
		}

		ecsact::entt::detail::add_system_markers_if_needed<C>(reg, entity);
		ecsact::entt::detail::add_exec_itr_beforechange_if_needed<C>(reg, entity);
	}

	return err;
}

template<typename C>
inline auto add_component_exec_options( //
	ecsact_registry_id                   registry_id,
	ecsact_entity_id                     entity_id,
	[[maybe_unused]] ecsact_component_id component_id,
	const void*                          component_data
) -> ecsact_add_error {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	auto err = ecsact::entt::check_add_component_error<C>(
		reg,
		entity,
		*static_cast<const C*>(component_data)
	);

	assert(err == ECSACT_ADD_OK);

	if(err == ECSACT_ADD_OK) {
		if constexpr(std::is_empty_v<C>) {
			reg.emplace<C>(entity);
		} else {
			reg.emplace<detail::exec_beforechange_storage<C>>(
				entity,
				*static_cast<const C*>(component_data)
			);
			reg.emplace<C>(entity, *static_cast<const C*>(component_data));
		}
		reg.template emplace_or_replace<component_added<C>>(entity);
		ecsact::entt::detail::add_system_markers_if_needed<C>(reg, entity);
		ecsact::entt::detail::add_exec_itr_beforechange_if_needed<C>(reg, entity);
	}

	return err;
}

template<typename C>
inline auto update_component( //
	ecsact_registry_id                       registry_id,
	ecsact_entity_id                         entity_id,
	[[maybe_unused]] ecsact_component_id     component_id,
	const void*                              component_data,
	ecsact::entt::detail::assoc_hash_value_t assoc_fields_hash
) -> ecsact_update_error {
	using ecsact::entt::detail::exec_beforechange_storage;

	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);
	static_assert(!std::is_empty_v<C>, "Tag components cannot be updated");

	auto err = ecsact::entt::check_update_component_error<C>(
		reg,
		entity,
		*static_cast<const C*>(component_data)
	);

	if(err != ECSACT_UPDATE_OK) {
		return err;
	}

	auto new_comp = static_cast<const C*>(component_data);

	if constexpr(C::has_assoc_fields) {
		auto new_assoc_fields_hash =
			ecsact::entt::detail::get_assoc_fields_hash(*new_comp);

		if(new_assoc_fields_hash == assoc_fields_hash) {
			auto storage_id = static_cast<::entt::id_type>(assoc_fields_hash);
			reg.template storage<C>(storage_id).get(entity) = *new_comp;
		} else {
			auto new_storage_id = static_cast<::entt::id_type>(new_assoc_fields_hash);
			auto old_storage_id = static_cast<::entt::id_type>(assoc_fields_hash);
			reg.template storage<C>(old_storage_id).erase(entity);
			reg.template storage<C>(new_storage_id).emplace(entity, *new_comp);
		}
	} else {
		reg.template get<C>(entity) = *new_comp;
	}

	return ECSACT_UPDATE_OK;
}

using update_component_exec_options_sig_t = ecsact_update_error (*)( //
	ecsact_registry_id                       registry_id,
	ecsact_entity_id                         entity_id,
	[[maybe_unused]] ecsact_component_id     component_id,
	const void*                              component_data,
	ecsact::entt::detail::assoc_hash_value_t assoc_fields_hash
);

template<typename C>
inline auto update_component_exec_options( //
	ecsact_registry_id                       registry_id,
	ecsact_entity_id                         entity_id,
	[[maybe_unused]] ecsact_component_id     component_id,
	const void*                              component_data,
	ecsact::entt::detail::assoc_hash_value_t assoc_fields_hash
) -> ecsact_update_error {
	using ecsact::entt::detail::exec_beforechange_storage;

	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);
	static_assert(!std::is_empty_v<C>, "Tag components cannot be updated");

	auto err = ecsact::entt::check_update_component_error<C>(
		reg,
		entity,
		*static_cast<const C*>(component_data)
	);

	if(err != ECSACT_UPDATE_OK) {
		return err;
	}

	const auto& in_component = *static_cast<const C*>(component_data);
	auto& beforechange = reg.template get<exec_beforechange_storage<C>>(entity);
	auto& current_component = reg.template get<C>(entity);

	if(!beforechange.has_update_occurred) {
		beforechange.value = current_component;
		beforechange.has_update_occurred = true;
	}
	current_component = in_component;

	return err;
}

static_assert(std::is_same_v<
							update_component_exec_options_sig_t,
							decltype(&update_component_exec_options<void>)>);

template<typename C>
auto remove_component(
	ecsact_registry_id                       registry_id,
	ecsact_entity_id                         entity_id,
	[[maybe_unused]] ecsact_component_id     component_id,
	ecsact::entt::detail::assoc_hash_value_t assoc_fields_hash
) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	if constexpr(C::has_assoc_fields) {
		auto storage_id = static_cast<::entt::id_type>(assoc_fields_hash);
		reg.storage<C>(storage_id).erase(entity);
	} else {
		reg.remove<C>(entity);
		if constexpr(!std::is_empty_v<C>) {
			reg.remove<detail::exec_beforechange_storage<C>>(entity);
		}
		reg.template remove<component_added<C>>(entity);
		reg.template emplace_or_replace<component_removed<C>>(entity);
		ecsact::entt::detail::remove_system_markers_if_needed<C>(reg, entity);
	}
}

using remove_component_exec_options_sig_t = void (*)( //
	ecsact_registry_id                       registry_id,
	ecsact_entity_id                         entity_id,
	[[maybe_unused]] ecsact_component_id     component_id,
	ecsact::entt::detail::assoc_hash_value_t assoc_fields_hash
);

template<typename C>
auto remove_component_exec_options(
	ecsact_registry_id                       registry_id,
	ecsact_entity_id                         entity_id,
	[[maybe_unused]] ecsact_component_id     component_id,
	ecsact::entt::detail::assoc_hash_value_t assoc_fields_hash
) -> void {
	using ecsact::entt::detail::pending_remove;

	auto& reg = ecsact::entt::get_registry(registry_id);
	auto  entity = ecsact::entt::entity_id{entity_id};
	assert(C::id == component_id);

	if constexpr(!std::is_empty_v<C>) {
		reg.template emplace_or_replace<detail::beforeremove_storage<C>>(
			entity,
			reg.template get<C>(entity)
		);
	}

	reg.template erase<C>(entity);
	reg.template remove<component_added<C>>(entity);
	reg.template emplace_or_replace<component_removed<C>>(entity);

	if constexpr(!std::is_empty_v<C>) {
		reg.template remove<detail::exec_beforechange_storage<C>>(entity);
	}

	ecsact::entt::detail::remove_system_markers_if_needed<C>(reg, entity);
}

static_assert(std::is_same_v<
							remove_component_exec_options_sig_t,
							decltype(&remove_component_exec_options<void>)>);

inline auto _trigger_create_entity_events(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	using ecsact::entt::detail::created_entity;

	auto& reg = ecsact::entt::get_registry(registry_id);

	if(events_collector.has_entity_created_callback()) {
		::entt::basic_view created_view{
			reg.template storage<created_entity>(),
		};

		for(ecsact::entt::entity_id entity : created_view) {
			events_collector.invoke_entity_created_callback(
				entity,
				created_view.template get<created_entity>(entity).placeholder_entity_id
			);
		}
	}

	reg.clear<created_entity>();
}

inline auto _trigger_destroy_entity_events(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	using ecsact::entt::detail::destroyed_entity;

	auto& reg = ecsact::entt::get_registry(registry_id);

	if(events_collector.has_entity_destroyed_callback()) {
		::entt::basic_view destroy_view{
			reg.template storage<destroyed_entity>(),
		};

		for(ecsact::entt::entity_id entity : destroy_view) {
			events_collector.invoke_entity_destroyed_callback(entity);
		}
	}

	reg.clear<destroyed_entity>();
}

template<typename C>
auto _trigger_init_component_event(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);

	if(!events_collector.has_init_callback()) {
		return;
	}

	if constexpr(C::transient) {
		return;
	}

	::entt::basic_view added_view{
		reg.template storage<C>(),
		reg.template storage<component_added<C>>(),
	};

	for(ecsact::entt::entity_id entity : added_view) {
		if constexpr(std::is_empty_v<C>) {
			events_collector.invoke_init_callback<C>(entity);
		} else {
			events_collector.invoke_init_callback<C>(
				entity,
				added_view.template get<C>(entity)
			);
		}
	}
}

template<typename C>
auto _trigger_update_component_event(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	using ecsact::entt::detail::beforeremove_storage;
	using ecsact::entt::detail::exec_beforechange_storage;

	//

	if(!events_collector.has_update_callback()) {
		return;
	}

	auto& reg = ecsact::entt::get_registry(registry_id);
	if constexpr(!C::transient && !std::is_empty_v<C>) {
		auto comp_view = reg.view<C, exec_beforechange_storage<C>>( //
			::entt::exclude<beforeremove_storage<C>>
		);

		for(ecsact::entt::entity_id entity : comp_view) {
			auto& before =
				comp_view.template get<exec_beforechange_storage<C>>(entity);
			auto& current = comp_view.template get<C>(entity);

			if(before.has_update_occurred && before.value != current) {
				events_collector.invoke_update_callback<C>(entity, current);
			}
			before.has_update_occurred = false;
		}
	}
}

template<typename C>
auto _trigger_remove_component_event(
	ecsact_registry_id                                registry_id,
	ecsact::entt::detail::execution_events_collector& events_collector
) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);

	if(!events_collector.has_remove_callback()) {
		return;
	}

	if constexpr(C::transient) {
		return;
	}

	if constexpr(std::is_empty_v<C>) {
		::entt::basic_view removed_view{
			reg.template storage<component_removed<C>>(),
		};
		for(ecsact::entt::entity_id entity : removed_view) {
			events_collector.invoke_remove_callback<C>(entity);
		}
	} else {
		::entt::basic_view removed_view{
			reg.template storage<detail::beforeremove_storage<C>>(),
			reg.template storage<component_removed<C>>(),
		};
		for(ecsact::entt::entity_id entity : removed_view) {
			events_collector.invoke_remove_callback<C>(
				entity,
				removed_view.template get<detail::beforeremove_storage<C>>(entity).value
			);
		}

		reg.template clear<detail::beforeremove_storage<C>>();
	}
}

auto check_action_error_t(
	ecsact_registry_id registry_id,
	const void*        action_data
) -> ecsact_execute_systems_error;

template<typename A>
inline auto check_action_error(
	ecsact_registry_id registry_id,
	const void*        action_data
) -> ecsact_execute_systems_error {
	auto& reg = ecsact::entt::get_registry(registry_id);

	auto action = *static_cast<const A*>(action_data);
	auto result = ecsact::entt::check_action_error(reg, action);

	return result;
}

template<typename C>
inline auto clear_component(ecsact_registry_id registry_id) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);

	reg.clear<ecsact::entt::component_added<C>>();
	reg.clear<ecsact::entt::component_removed<C>>();
}

template<typename S>
inline auto clear_notify_component(ecsact_registry_id registry_id) -> void {
	auto& reg = ecsact::entt::get_registry(registry_id);

	reg.clear<ecsact::entt::detail::run_system<S>>();
}

template<typename C>
inline auto prepare_component(ecsact_registry_id registry_id) -> void {
	using namespace ecsact::entt;

	auto& reg = ecsact::entt::get_registry(registry_id);

	reg.template storage<C>();
	reg.template storage<component_added<C>>();
	reg.template storage<component_removed<C>>();
	reg.template storage<detail::pending_add<C>>();
	reg.template storage<detail::pending_remove<C>>();
	reg.template storage<detail::beforeremove_storage<C>>();

	if constexpr(!std::is_empty_v<C>) {
		reg.template storage<detail::exec_beforechange_storage<C>>();
		reg.template storage<detail::exec_itr_beforechange_storage<C>>();
	}
}

template<typename S>
inline auto prepare_system(ecsact_registry_id registry_id) -> void {
	using namespace ecsact::entt::detail;
	auto& reg = ecsact::entt::get_registry(registry_id);

	reg.template storage<system_sorted<S>>();
	reg.template storage<pending_lazy_execution<S>>();
	reg.template storage<run_system<S>>();
}

template<typename C, typename V>
auto has_component_changed(entt::entity_id entity, V& view) -> bool {
	using detail::exec_itr_beforechange_storage;

	const auto& current_comp = view.template get<C>(entity);
	const auto& before_comp =
		view.template get<exec_itr_beforechange_storage<C>>(entity);

	if(before_comp.value != current_comp) {
		return true;
	}
	return false;
}

template<typename C>
auto update_exec_itr_beforechange(
	entt::entity_id           entity,
	ecsact::entt::registry_t& reg
) -> void {
	auto  comp = reg.get<C>(entity);
	auto& beforechange_comp =
		reg.get<detail::exec_itr_beforechange_storage<C>>(entity);

	beforechange_comp.value = comp;
}

} // namespace ecsact::entt::wrapper::core
