#pragma once

#include <optional>
#include <utility>
#include <mutex>
#include <entt/entt.hpp>
#include <ecsact/runtime.hh>
#include <ecsact/runtime/common.h>
#include <ecsact/runtime/core.h>

#include "runtime-util/runtime-util.hh"

namespace ecsact_entt_rt {
	using entity_id_map_t = std::unordered_map
		< ::ecsact::entity_id
		, typename ::entt::registry::entity_type
		>;
	using add_component_callback_set_t = ::ecsact::runtime_util::callback_set
		< ecsact_add_component_callback
		, void*
		>;
	using add_component_callbacks_t = std::unordered_map
		< ::ecsact::component_id
		, add_component_callback_set_t
		>;

	using update_component_callback_set_t = ::ecsact::runtime_util::callback_set
		< ecsact_update_component_callback
		, void*
		>;
	using update_component_callbacks_t = std::unordered_map
		< ::ecsact::component_id
		, update_component_callback_set_t
		>;

	using before_remove_component_callback_set_t =
		::ecsact::runtime_util::callback_set
			< ecsact_before_remove_component_callback
			, void*
			>;
	using before_remove_component_callbacks_t = std::unordered_map
		< ::ecsact::component_id
		, before_remove_component_callback_set_t
		>;

	using after_remove_component_callback_set_t =
		::ecsact::runtime_util::callback_set
			< ecsact_after_remove_component_callback
			, void*
			>;
	using after_remove_component_callbacks_t = std::unordered_map
		< ::ecsact::component_id
		, after_remove_component_callback_set_t
		>;

	using add_any_comp_cbs_t = ::ecsact::runtime_util::callback_set
		< ecsact_add_any_component_callback
		, void*
		>;
	using update_any_comp_cbs_t = ::ecsact::runtime_util::callback_set
		< ecsact_update_any_component_callback
		, void*
		>;
	using before_remove_any_comp_cbs_t = ::ecsact::runtime_util::callback_set
		< ecsact_before_remove_any_component_callback
		, void*
		>;
	using after_remove_any_comp_cbs_t = ::ecsact::runtime_util::callback_set
		< ecsact_after_remove_any_component_callback
		, void*
		>;


	template<typename Package>
	struct registry_info {
		std::optional<std::reference_wrapper<std::mutex>> mutex;
		::entt::registry registry;
		entity_id_map_t entities_map;
		/**
		 * Index of this vector is a statically casted EnTT ID
		 */
		std::vector<::ecsact::entity_id> _ecsact_entity_ids;

		::ecsact::entity_id last_entity_id{};

		add_component_callbacks_t add_component_callbacks;
		update_component_callbacks_t update_component_callbacks;
		before_remove_component_callbacks_t before_remove_component_callbacks;
		after_remove_component_callbacks_t after_remove_component_callbacks;

		add_any_comp_cbs_t add_any_component_callbacks;
		update_any_comp_cbs_t update_any_component_callbacks;
		before_remove_any_comp_cbs_t before_remove_any_component_callbacks;
		after_remove_any_comp_cbs_t after_remove_any_component_callbacks;
		
		using actions_tuple_t = boost::mp11::mp_assign
			< std::tuple<>
			, typename Package::actions
			>;

		using actions_t = boost::mp11::mp_transform
			< std::vector
			, actions_tuple_t
			>;
		
		actions_t actions;

		struct create_new_entity_result {
			typename ::entt::registry::entity_type entt_entity_id;
			::ecsact::entity_id ecsact_entity_id;
		};

		/** @internal */
		inline auto _create_entity
			( ::ecsact::entity_id ecsact_entity_id
			)
		{
			auto new_entt_entity_id = registry.create();
			entities_map[ecsact_entity_id] = new_entt_entity_id;
			_ecsact_entity_ids.resize(static_cast<size_t>(new_entt_entity_id) + 1);
			_ecsact_entity_ids[_ecsact_entity_ids.size() - 1] = ecsact_entity_id;
			return new_entt_entity_id;
		}

		/** @internal */
		inline create_new_entity_result _create_entity() {
			auto new_entity_id = static_cast<::ecsact::entity_id>(
				static_cast<int>(last_entity_id) + 1
			);
			while(entities_map.contains(new_entity_id)) {
				new_entity_id = static_cast<::ecsact::entity_id>(
					static_cast<int>(new_entity_id) + 1
				);
			}
			last_entity_id = new_entity_id;
			return {
				.entt_entity_id = _create_entity(new_entity_id),
				.ecsact_entity_id = new_entity_id,
			};
		}
	
		// Creates an entity and also makes sure there is a matching one in the
		// pending registry
		inline auto create_entity
			( ::ecsact::entity_id ecsact_entity_id
			)
		{
			std::scoped_lock lk(mutex->get());
			return _create_entity(ecsact_entity_id);
		}
		inline auto create_entity() {
			std::scoped_lock lk(mutex->get());
			return _create_entity();
		}

		typename ::entt::registry::entity_type entt_entity_id
			( ::ecsact::entity_id ecsact_entity_id
			) const
		{
			return entities_map.at(ecsact_entity_id);
		}

		::ecsact::entity_id ecsact_entity_id
			( typename ::entt::registry::entity_type entt_entity_id
			) const
		{
			return _ecsact_entity_ids.at(static_cast<size_t>(entt_entity_id));
		}
	};
}
