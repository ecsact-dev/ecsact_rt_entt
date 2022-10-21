#pragma once

#include <type_traits>
#include <optional>
#include <utility>
#include <mutex>
#include <boost/mp11.hpp>
#include <entt/entt.hpp>
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/core.h"

#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/detail/internal_markers.hh"

namespace ecsact_entt_rt {
using entity_id_map_t = std::unordered_map<ecsact_entity_id, entt::entity>;

template<typename Package>
struct registry_info {
	using package = Package;

	std::optional<std::reference_wrapper<std::mutex>> mutex;
	::entt::registry                                  registry;
	entity_id_map_t                                   entities_map;

	/**
	 * Index of this vector is a statically casted EnTT ID
	 */
	std::vector<ecsact_entity_id> _ecsact_entity_ids;

	ecsact_entity_id last_entity_id{};

	struct create_new_entity_result {
		entt::entity     entt_entity_id;
		ecsact_entity_id ecsact_entity_id;
	};

	void init_registry() {
		using boost::mp11::mp_for_each;
		using ecsact::entt::component_added;
		using ecsact::entt::component_changed;
		using ecsact::entt::component_removed;
		using ecsact::entt::detail::beforechange_storage;
		using ecsact::entt::detail::temp_storage;

		mp_for_each<typename package::components>([&]<typename C>(C) {
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

		auto entity_field = field.template get<ecsact_entity_id>(component);
		auto entity_field_entt = entities_map.at(entity_field);
		mp_with_index<64>(field.offset, [&](auto I) {
			registry.emplace<association<C, I>>(entity_field_entt);
		});
	}

	template<typename C>
	void _remove_association(
		const C&                  component,
		const ecsact::field_info& field
	) {
		using boost::mp11::mp_with_index;
		using ecsact::entt::detail::association;

		auto entity_field = field.template get<ecsact_entity_id>(component);
		auto entity_field_entt = entities_map.at(entity_field);
		mp_with_index<64>(field.offset, [&](auto I) {
			registry.erase<association<C, I>>(entity_field_entt);
		});
	}

	template<typename C>
		requires(std::is_empty_v<C>)
	void add_component(::entt::entity entity) {
		registry.emplace<C>(entity);
	}

	template<typename C, typename... Args>
		requires(!std::is_empty_v<C>)
	void add_component(::entt::entity entity, Args&&... args) {
		using boost::mp11::mp_for_each;
		using boost::mp11::mp_with_index;

		auto& comp = registry.emplace<C>(entity, std::forward<Args>(args)...);

		mp_for_each<typename package::components>([&]<typename O>(O) {
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
	void remove_component(::entt::entity entity) {
		using boost::mp11::mp_for_each;

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
			mp_for_each<typename package::components>([&]<typename O>(O) {
				if constexpr(std::is_same_v<std::remove_cvref_t<C>, O>) {
					using ecsact::entt::detail::beforechange_storage;
					registry.erase<beforechange_storage<O>>(entity);
				}
			});
		}
	}

	/** @internal */
	inline auto _create_entity(ecsact_entity_id ecsact_entity_id) {
		auto new_entt_entity_id = registry.create();
		entities_map[ecsact_entity_id] = new_entt_entity_id;
		_ecsact_entity_ids.resize(static_cast<size_t>(new_entt_entity_id) + 1);
		_ecsact_entity_ids[_ecsact_entity_ids.size() - 1] = ecsact_entity_id;
		return new_entt_entity_id;
	}

	/** @internal */
	inline create_new_entity_result _create_entity() {
		auto new_entity_id =
			static_cast<ecsact_entity_id>(static_cast<int>(last_entity_id) + 1);
		while(entities_map.contains(new_entity_id)) {
			new_entity_id =
				static_cast<ecsact_entity_id>(static_cast<int>(new_entity_id) + 1);
		}
		last_entity_id = new_entity_id;
		return {
			.entt_entity_id = _create_entity(new_entity_id),
			.ecsact_entity_id = new_entity_id,
		};
	}

	// Creates an entity and also makes sure there is a matching one in the
	// pending registry
	inline auto create_entity(ecsact_entity_id ecsact_entity_id) {
		std::scoped_lock lk(mutex->get());
		return _create_entity(ecsact_entity_id);
	}

	inline auto create_entity() {
		std::scoped_lock lk(mutex->get());
		return _create_entity();
	}

	entt::entity get_entt_entity_id(ecsact_entity_id ecsact_entity_id) const {
		return entities_map.at(ecsact_entity_id);
	}

	ecsact_entity_id get_ecsact_entity_id(entt::entity entt_entity_id) const {
		return _ecsact_entity_ids.at(static_cast<size_t>(entt_entity_id));
	}
};
} // namespace ecsact_entt_rt
