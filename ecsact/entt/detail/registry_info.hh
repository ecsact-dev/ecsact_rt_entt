#pragma once

#include <type_traits>
#include <optional>
#include <utility>
#include <mutex>
#include <boost/mp11.hpp>
#include <entt/entt.hpp>
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/core.h"
#include "ecsact/entt/detail/meta_util.hh"
#include "ecsact/cpp/type_info.hh"
#include "ecsact/entt/system_view.hh"
#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/entity.hh"
#include "ecsact/entt/detail/internal_markers.hh"

#include "meta_util.hh"

namespace ecsact_entt_rt {

template<typename Package>
struct registry_info {
	using package = Package;

	std::optional<std::reference_wrapper<std::mutex>> mutex;
	::entt::registry                                  registry;

	void init_registry() {
		using ecsact::entt::component_added;
		using ecsact::entt::component_changed;
		using ecsact::entt::component_removed;
		using ecsact::entt::detail::beforechange_storage;
		using ecsact::entt::detail::mp_for_each_available_component;
		using ecsact::entt::detail::temp_storage;

		mp_for_each_available_component<package>([&]<typename C>(C) {
			registry.storage<C>();
			registry.storage<temp_storage<C>>();
			registry.storage<component_added<C>>();
			registry.storage<component_changed<C>>();
			registry.storage<component_removed<C>>();
			if constexpr(!std::is_empty_v<C>) {
				registry.storage<beforechange_storage<C>>();
			}
		});
	}

	template<typename C>
	void _add_association(const C& component, const ecsact::field_info& field) {
		using boost::mp11::mp_with_index;
		using ecsact::entt::detail::association;

		ecsact::entt::entity_id entity_field =
			field.template get<ecsact_entity_id>(&component);

		// TODO(zaucy): Increasing the mp_with_index number causes really long
		//              compile times. Iterating over the available associations
		//              would perform better here.
		assert(field.offset < 32);
		mp_with_index<32>(field.offset, [&](auto I) {
			if(registry.all_of<association<C, I>>(entity_field)) {
				auto& assoc_comp = registry.get<association<C, I>>(entity_field);
				assoc_comp.ref_count += 1;
			} else {
				registry.emplace<association<C, I>>(entity_field, 1);
			}
		});
	}

	template<typename C>
	void _remove_association(
		const C&                  component,
		const ecsact::field_info& field
	) {
		using boost::mp11::mp_with_index;
		using ecsact::entt::detail::association;

		ecsact::entt::entity_id entity_field =
			field.template get<ecsact_entity_id>(&component);

		assert(field.offset < 32);
		// TODO(zaucy): Increasing the mp_with_index number causes really long
		//              compile times. Iterating over the available associations
		//              would perform better here.
		mp_with_index<32>(field.offset, [&](auto I) {
			auto& assoc_comp = registry.get<association<C, I>>(entity_field);
			assoc_comp.ref_count -= 1;
			if(assoc_comp.ref_count == 0) {
				registry.erase<association<C, I>>(entity_field);
			}
		});
	}

	template<typename C>
		requires(std::is_empty_v<C>)
	void add_component(ecsact::entt::entity_id entity) {
		registry.emplace<C>(entity);
	}

	template<typename C, typename... Args>
		requires(!std::is_empty_v<C>)
	void add_component(ecsact::entt::entity_id entity, Args&&... args) {
		using boost::mp11::mp_with_index;
		using ecsact::entt::detail::mp_for_each_available_component;

		auto& comp = registry.emplace<C>(entity, std::forward<Args>(args)...);

		mp_for_each_available_component<package>([&]<typename O>(O) {
			if constexpr(std::is_same_v<std::remove_cvref_t<C>, O>) {
				using ecsact::entt::detail::beforechange_storage;
				beforechange_storage<O> beforechange = {
					.value{std::forward<Args>(args)...},
					.set = false,
				};
				registry.emplace<beforechange_storage<O>>(
					entity,
					std::move(beforechange)
				);
			}
		});

		constexpr auto fields_info = ecsact::fields_info<C>();
		if constexpr(!fields_info.empty()) {
			for(auto& field : fields_info) {
				if(field.storage_type == ECSACT_ENTITY_TYPE) {
					_add_association(comp, field);
				}
			}
		}
	}

	template<typename C>
	void remove_component(ecsact::entt::entity_id entity) {
		using ecsact::entt::detail::mp_for_each_available_component;

		constexpr auto fields_info = ecsact::fields_info<C>();
		if constexpr(!fields_info.empty()) {
			auto& comp = registry.get<C>(entity);
			for(auto& field : fields_info) {
				if(field.storage_type == ECSACT_ENTITY_TYPE) {
					_remove_association(comp, field);
				}
			}
		}

		registry.erase<C>(entity);

		if constexpr(!std::is_empty_v<C>) {
			mp_for_each_available_component<package>([&]<typename O>(O) {
				if constexpr(std::is_same_v<std::remove_cvref_t<C>, O>) {
					using ecsact::entt::detail::beforechange_storage;
					registry.erase<beforechange_storage<O>>(entity);
				}
			});
		}
	}

	/** @internal */
	inline auto _create_entity( //
		ecsact::entt::entity_id entity
	) -> ecsact::entt::entity_id {
		if(registry.valid(entity)) {
			return entity;
		}

		auto new_entity = registry.create(entity.as_entt());
		// Our valid check above should have allowed this to happen
		assert(new_entity == entity.as_entt());
		return new_entity;
	}

	/** @internal */
	inline auto _create_entity() -> ecsact::entt::entity_id {
		return registry.create();
	}

	inline auto create_entity( //
		ecsact::entt::entity_id ecsact_entity_id
	) -> ecsact::entt::entity_id {
		std::scoped_lock lk(mutex->get());
		return _create_entity(ecsact_entity_id);
	}

	inline auto create_entity() {
		std::scoped_lock lk(mutex->get());
		return _create_entity();
	}

	inline void destroy_entity(ecsact::entt::entity_id entity_id) {
		registry.destroy(entity_id);
	}
};
} // namespace ecsact_entt_rt
