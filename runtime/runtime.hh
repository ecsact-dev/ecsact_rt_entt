#pragma once

#include <cassert>
#include <vector>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <tuple>
#include <cstdio>
#include <boost/mp11.hpp>
#include <ecsact/runtime.hh>
#include <ecsact/runtime/common.h>
#include <ecsact/runtime/core.h>
#include <ecsact/lib.hh>
#include <entt/entt.hpp>

#include "runtime-util/runtime-util.hh"

namespace ecsact::entt::details {
	template<typename C> requires(std::is_empty_v<C>)
	struct temp_storage { };

	template<typename C> requires(!std::is_empty_v<C>)
	struct temp_storage { C value; };

	template<typename C> requires(std::is_empty_v<C>)
	struct beforechange_storage { };

	template<typename C> requires(!std::is_empty_v<C>)
	struct beforechange_storage { C value; };
}

namespace ecsact::entt {

	/**
	 * Marker to indicate that a component has been added
	 */
	template<typename Component>
	struct component_added {};

	/**
	 * Marker to indicate that a component has been removed
	 */
	template<typename Component>
	struct component_removed {};

	template<::ecsact::package Package>
	class runtime {
		friend struct ::ecsact_system_execution_context;
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

		/**
		 * Checks if type T is listd as one of the actions in the ecact package.
		 * @returns `true` if T is a component belonging to `package`, `false` 
		 *          otherwise.
		 */
		template<typename T>
		static constexpr bool is_action() {
			using boost::mp11::mp_bind_front;
			using boost::mp11::mp_transform_q;
			using boost::mp11::mp_any;
			using boost::mp11::mp_apply;

			return mp_apply<mp_any, mp_transform_q<
				mp_bind_front<std::is_same, std::remove_cvref_t<T>>,
				typename Package::actions
			>>::value;
		}

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

		using registries_map_t = std::unordered_map
			< ::ecsact::registry_id
			, registry_info
			>;

		::ecsact::registry_id _last_registry_id{};
		registries_map_t _registries;

		template<typename ComponentT>
		void _invoke_add_callbacks
			( registry_info&       info
			, ::ecsact::entity_id  entity
			, const ComponentT&    component
			)
		{
			if(info.add_component_callbacks.contains(ComponentT::id)) {
				info.add_component_callbacks.at(ComponentT::id)(
					static_cast<ecsact_entity_id>(entity),
					static_cast<const void*>(&component)
				);
			}

			info.add_any_component_callbacks(
				static_cast<ecsact_entity_id>(entity),
				static_cast<ecsact_component_id>(ComponentT::id),
				static_cast<const void*>(&component)
			);
		}

		template<typename ComponentT>
		void _invoke_update_callbacks
			( registry_info&       info
			, ::ecsact::entity_id  entity
			, const ComponentT&    component
			)
		{
			if(info.update_component_callbacks.contains(ComponentT::id)) {
				info.update_component_callbacks.at(ComponentT::id)(
					static_cast<ecsact_entity_id>(entity),
					static_cast<const void*>(&component)
				);
			}

			info.update_any_component_callbacks(
				static_cast<ecsact_entity_id>(entity),
				static_cast<ecsact_component_id>(ComponentT::id),
				static_cast<const void*>(&component)
			);
		}

		template<typename ComponentT>
		void _invoke_before_remove_callbacks
			( registry_info&                                  info
			, ::ecsact::entity_id                             entity_id
			, typename ::entt::registry::entity_type  entt_entity_id
			, auto&&                                          component_source
			)
		{
			if(info.before_remove_component_callbacks.contains(ComponentT::id)) {
				if constexpr(std::is_empty_v<ComponentT>) {
					ComponentT c{};
					info.before_remove_component_callbacks.at(ComponentT::id)(
						static_cast<ecsact_entity_id>(entity_id),
						&c
					);
				} else {
					auto& component = component_source.get<ComponentT>(entt_entity_id);
					info.before_remove_component_callbacks.at(ComponentT::id)(
						static_cast<ecsact_entity_id>(entity_id),
						static_cast<const void*>(&component)
					);
				}
			}

			if(!info.before_remove_any_component_callbacks.empty()) {
				if constexpr(std::is_empty_v<ComponentT>) {
					ComponentT c{};
					info.before_remove_any_component_callbacks(
						static_cast<ecsact_entity_id>(entity_id),
						static_cast<ecsact_component_id>(ComponentT::id),
						&c
					);
				} else {
					auto& component = component_source.get<ComponentT>(entt_entity_id);
					info.before_remove_any_component_callbacks(
						static_cast<ecsact_entity_id>(entity_id),
						static_cast<ecsact_component_id>(ComponentT::id),
						static_cast<const void*>(&component)
					);
				}
			}
		}

		template<typename ComponentT>
		void _invoke_before_remove_callbacks
			( registry_info&                                  info
			, typename ::entt::registry::entity_type  entt_entity_id
			, auto&&                                          component_source
			)
		{
			auto ecsact_entity_id = info.ecsact_entity_id(entt_entity_id);
			_invoke_before_remove_callbacks<ComponentT>(
				info,
				ecsact_entity_id,
				entt_entity_id,
				info.registry
			);
		}

		template<typename ComponentT>
		void _invoke_before_remove_callbacks
			( registry_info&       info
			, ::ecsact::entity_id  ecsact_entity_id
			)
		{
			auto entt_entity_id = info.entities_map.at(ecsact_entity_id);
			_invoke_before_remove_callbacks<ComponentT>(
				info,
				ecsact_entity_id,
				entt_entity_id,
				info.registry
			);
		}

		template<typename ComponentT>
		void _invoke_after_remove_callbacks
			( registry_info&       info
			, ::ecsact::entity_id  entity
			)
		{
			if(info.after_remove_component_callbacks.contains(ComponentT::id)) {
				info.after_remove_component_callbacks.at(ComponentT::id)(
					static_cast<ecsact_entity_id>(entity)
				);
			}

			info.after_remove_any_component_callbacks(
				static_cast<ecsact_entity_id>(entity),
				static_cast<ecsact_component_id>(ComponentT::id)
			);
		}

	public:
		using registry_type = ::entt::registry;
		using entt_entity_type = typename registry_type::entity_type;
		using package = Package;

		::ecsact::registry_id create_registry
			( const char* registry_name
			)
		{
			using boost::mp11::mp_for_each;

			// Using the index of _registries as an ID
			const auto reg_id = static_cast<::ecsact::registry_id>(
				static_cast<int>(_last_registry_id) + 1
			);

			_last_registry_id = reg_id;

			auto itr = _registries.emplace_hint(
				_registries.end(),
				std::piecewise_construct,
				std::forward_as_tuple(reg_id),
				std::forward_as_tuple()
			);

			registry_info& info = itr->second;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				using namespace ::entt::literals;
				static_cast<void>(info.registry.storage<C>());

				if constexpr(!C::transient) {
					static_cast<void>(info.registry.storage<C>("temp"_hs));
					static_cast<void>(info.registry.storage<component_added<C>>());
					static_cast<void>(info.registry.storage<component_removed<C>>());
				}
			});

			mp_for_each<typename package::system_writables>([&]<typename C>(C) {
				using namespace ::entt::literals;
				static_cast<void>(info.registry.storage<C>("beforechange"_hs));
			});

			return reg_id;
		}

		void destroy_registry
			( ::ecsact::registry_id reg_id
			)
		{
			_registries.erase(reg_id);
		}

		void clear_registry
			( ::ecsact::registry_id reg_id
			)
		{
			using boost::mp11::mp_for_each;

			auto& info = _registries.at(reg_id);
			std::vector<std::function<void(registry_info&)>> after_remove_fns;

			for(auto&& [entity, entt_entity_id] : info.entities_map) {
				mp_for_each<typename package::components>([&]<typename C>(const C&) {
					if(!info.registry.all_of<C>(entt_entity_id)) return;
					_invoke_before_remove_callbacks<C>(info, entity);
					after_remove_fns.emplace_back([this, entity](auto& info) {
						_invoke_after_remove_callbacks<C>(info, entity);
					});
				});
			}

			info.registry.clear();
			info.entities_map.clear();

			for(auto& fn : after_remove_fns) {
				fn(info);
			}
		}

		::ecsact::entity_id create_entity
			( ::ecsact::registry_id reg_id
			)
		{
			std::mutex mutex;
			auto& info = _registries.at(reg_id);
			info.mutex = mutex;
			auto new_entity_id = info.create_entity().ecsact_entity_id;
			info.mutex = std::nullopt;
			return new_entity_id;
		}

		void ensure_entity
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			if(!info.entities_map.contains(entity_id)) {
				std::mutex mutex;
				info.mutex = mutex;
				info.create_entity(entity_id);
				info.mutex = std::nullopt;
			}
		}

		bool entity_exists
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			return info.entities_map.contains(entity_id);
		}

		void destroy_entity
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			using boost::mp11::mp_for_each;

			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);
			std::vector<std::function<void(registry_info&)>> after_remove_fns;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(!info.registry.all_of<C>(entt_entity_id)) return;
				_invoke_before_remove_callbacks<C>(info, entity_id);
				after_remove_fns.emplace_back([this, entity_id](auto& info) {
					_invoke_after_remove_callbacks<C>(info, entity_id);
				});
			});

			info.registry.destroy(entt_entity_id);
			info.entities_map.erase(entity_id);

			for(auto& fn : after_remove_fns) {
				fn(info);
			}
		}

		int count_entities
			( ::ecsact::registry_id reg_id
			)
		{
			auto& info = _registries.at(reg_id);
			return static_cast<int>(info.registry.size());
		}

		std::vector<ecsact::entity_id> get_entities
			( ::ecsact::registry_id reg_id
			)
		{
			auto& info = _registries.at(reg_id);
			std::vector<ecsact::entity_id> result;
			for(auto& entry: info.entities_map) {
				result.push_back(entry.first);
			}

			return result;
		}

		void get_entities
			( ::ecsact::registry_id  reg_id
			, int                    max_entities_count
			, ::ecsact::entity_id*   out_entities
			, int*                   out_entities_count
			)
		{
			auto& info = _registries.at(reg_id);

			int entities_count = static_cast<int>(info.entities_map.size());
			max_entities_count = std::min(entities_count, max_entities_count);

			auto itr = info.entities_map.begin();
			for(int i=0; max_entities_count > i; ++i) {
				if(itr == info.entities_map.end()) break;

				out_entities[i] = itr->first;
				++itr;
			}

			if(out_entities_count != nullptr) {
				*out_entities_count = entities_count;
			}
		}

		template<typename ComponentT>
		void add_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			, const ComponentT&      component_data
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			if constexpr(std::is_empty_v<ComponentT>) {
				info.registry.emplace<ComponentT>(entt_entity_id);
				_invoke_add_callbacks<ComponentT>(info, entity_id, ComponentT{});
			} else {
				auto& component = info.registry.emplace<ComponentT>(
					entt_entity_id,
					component_data
				);

				_invoke_add_callbacks<ComponentT>(info, entity_id, component);
			}
		}

		template<typename ComponentT>
		void add_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			add_component<ComponentT>(reg_id, entity_id, ComponentT{});
		}

		void add_component
			( ::ecsact::registry_id   reg_id
			, ::ecsact::entity_id     entity_id
			, ::ecsact::component_id  component_id
			, const void*             component_data
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					if constexpr(std::is_empty_v<C>) {
						add_component<C>(reg_id, entity_id);
					} else {
						add_component<C>(
							reg_id,
							entity_id,
							*static_cast<const C*>(component_data)
						);
					}
				}
			});
		}

		template<typename ComponentT>
		bool has_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			return info.registry.all_of<ComponentT>(entt_entity_id);
		}

		bool has_component
			( ::ecsact::registry_id   reg_id
			, ::ecsact::entity_id     entity_id
			, ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			bool result = false;
			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					result = has_component<C>(reg_id, entity_id);
				}
			});
			return result;
		}

		template<typename ComponentT>
		const ComponentT& get_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			return info.registry.get<ComponentT>(entt_entity_id);
		}

		const void* get_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			, ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			const void* component_data = nullptr;
			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					if constexpr(std::is_empty_v<C>) {
						static C c{};
						component_data = &c;
					} else {
						component_data = &(get_component<C>(reg_id, entity_id));
					}
				}
			});
			return component_data;
		}

		template<typename ComponentT>
		void update_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			, const ComponentT&      component_data
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);
	
			auto& component = info.registry.get<ComponentT>(entt_entity_id);
			component = component_data;
			_invoke_update_callbacks<ComponentT>(info, entity_id, component);
		}

		void update_component
			( ::ecsact::registry_id   reg_id
			, ::ecsact::entity_id     entity_id
			, ::ecsact::component_id  component_id
			, const void*             component_data
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					if constexpr(!std::is_empty_v<C>) {
						update_component<C>(
							reg_id,
							entity_id,
							*static_cast<const C*>(component_data)
						);
					}
				}
			});
		}

		template<typename ComponentT>
		void remove_component
			( ::ecsact::registry_id  reg_id
			, ::ecsact::entity_id    entity_id
			)
		{
			auto& info = _registries.at(reg_id);
			auto entt_entity_id = info.entities_map.at(entity_id);

			info.registry.remove<ComponentT>(entt_entity_id);
		}

		void remove_component
			( ::ecsact::registry_id   reg_id
			, ::ecsact::entity_id     entity_id
			, ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					remove_component<C>(reg_id, entity_id);
				}
			});
		}

		template<typename ComponentT>
		void on_add_component
			( ::ecsact::registry_id          reg_id
			, ecsact_add_component_callback  callback
			, void*                          callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_component_callbacks[ComponentT::id].add(
				callback,
				callback_user_data
			);
		}

		void on_add_component
			( ::ecsact::registry_id          reg_id
			, ::ecsact::component_id         component_id
			, ecsact_add_component_callback  callback
			, void*                          callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_component_callbacks[component_id].add(
				callback,
				callback_user_data
			);
		}

		void off_add_component
			( ::ecsact::registry_id          reg_id
			, ::ecsact::component_id         component_id
			, ecsact_add_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_component_callbacks[component_id].remove(callback);
		}

		template<typename ComponentT>
		void on_update_component
			( ::ecsact::registry_id             reg_id
			, ecsact_update_component_callback  callback
			, void*                             callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_component_callbacks[ComponentT::id].add(
				callback,
				callback_user_data
			);
		}

		void on_update_component
			( ::ecsact::registry_id             reg_id
			, ::ecsact::component_id            component_id
			, ecsact_update_component_callback  callback
			, void*                             callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_component_callbacks[component_id].add(
				callback,
				callback_user_data
			);
		}

		void off_update_component
			( ::ecsact::registry_id             reg_id
			, ::ecsact::component_id            component_id
			, ecsact_update_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_component_callbacks[component_id].remove(callback);
		}

		template<typename ComponentT>
		void on_before_remove_component
			( ::ecsact::registry_id                    reg_id
			, ecsact_before_remove_component_callback  callback
			, void*                                    callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_component_callbacks[ComponentT::id].add(
				callback,
				callback_user_data
			);
		}

		void on_before_remove_component
			( ::ecsact::registry_id                    reg_id
			, ::ecsact::component_id                   component_id
			, ecsact_before_remove_component_callback  callback
			, void*                                    callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_component_callbacks[component_id].add(
				callback,
				callback_user_data
			);
		}

		void off_before_remove_component
			( ::ecsact::registry_id                    reg_id
			, ::ecsact::component_id                   component_id
			, ecsact_before_remove_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_component_callbacks[component_id].remove(callback);
		}

		template<typename ComponentT>
		void on_after_remove_component
			( ::ecsact::registry_id                   reg_id
			, ecsact_after_remove_component_callback  callback
			, void*                                   callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_component_callbacks[ComponentT::id].add(
				callback,
				callback_user_data
			);
		}

		void on_after_remove_component
			( ::ecsact::registry_id                   reg_id
			, ::ecsact::component_id                  component_id
			, ecsact_after_remove_component_callback  callback
			, void*                                   callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_component_callbacks[component_id].add(
				callback,
				callback_user_data
			);
		}

		void off_after_remove_component
			( ::ecsact::registry_id                   reg_id
			, ::ecsact::component_id                  component_id
			, ecsact_after_remove_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_component_callbacks[component_id].remove(callback);
		}

		void on_add_any_component
			( ::ecsact::registry_id              reg_id
			, ecsact_add_any_component_callback  callback
			, void*                              callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_any_component_callbacks.add(
				callback,
				callback_user_data
			);
		}

		void off_add_any_component
			( ::ecsact::registry_id              reg_id
			, ecsact_add_any_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.add_any_component_callbacks.remove(callback);
		}

		void on_update_any_component
			( ::ecsact::registry_id                 reg_id
			, ecsact_update_any_component_callback  callback
			, void*                                 callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_any_component_callbacks.add(
				callback,
				callback_user_data
			);
		}

		void off_update_any_component
			( ::ecsact::registry_id                 reg_id
			, ecsact_update_any_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.update_any_component_callbacks.remove(callback);
		}

		void on_before_remove_any_component
			( ::ecsact::registry_id                        reg_id
			, ecsact_before_remove_any_component_callback  callback
			, void*                                        callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_any_component_callbacks.add(
				callback,
				callback_user_data
			);
		}

		void off_before_remove_any_component
			( ::ecsact::registry_id                        reg_id
			, ecsact_before_remove_any_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.before_remove_any_component_callbacks.remove(callback);
		}

		void on_after_remove_any_component
			( ::ecsact::registry_id                       reg_id
			, ecsact_after_remove_any_component_callback  callback
			, void*                                       callback_user_data
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_any_component_callbacks.add(
				callback,
				callback_user_data
			);
		}

		void off_after_remove_any_component
			( ::ecsact::registry_id                       reg_id
			, ecsact_after_remove_any_component_callback  callback
			)
		{
			auto& info = _registries.at(reg_id);
			info.after_remove_any_component_callbacks.remove(callback);
		}

		template<typename ActionT>
		void push_action
			( ::ecsact::registry_id  reg_id
			, const ActionT&         action
			)
		{
			auto& info = _registries.at(reg_id);
			std::get<std::vector<ActionT>>(info.actions).push_back(action);
		}

		void push_action
			( ::ecsact::registry_id  reg_id
			, ::ecsact::action_id    system_id
			, const void*            action_data
			)
		{
			using boost::mp11::mp_for_each;
			using boost::mp11::mp_identity;
			using boost::mp11::mp_transform;

			mp_for_each<mp_transform<mp_identity, typename package::actions>>(
				[&]<typename A>(mp_identity<A>) {
					if(A::id == system_id) {
						if constexpr(std::is_empty_v<A>) {
							push_action<A>(reg_id, A{});
						} else {
							push_action<A>(reg_id, *static_cast<const A*>(action_data));
						}
					}
				}
			);
		}

	private:
		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial_removes_only
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename SystemT::removes>([&]<typename C>(C) {
				info.registry.clear<C>();
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial_default
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			)
		{
			using boost::mp11::mp_for_each;

			auto view = ecsact_entt_view(
				std::type_identity<SystemT>{},
				info.registry
			);

			std::for_each(view.begin(), view.end(), [&](auto entity) {
				ecsact_system_execution_context ctx{
					.info = info,
					.entity = entity,
					.parent = parent,
					.action = action,
				};

				mp_for_each<typename SystemT::removes>([&]<typename C>(C) {
					ctx.remove<C>();
				});
				mp_for_each<typename SystemT::adds>([&]<typename C>(C) {
					ctx.add<C>(C{});
				});
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_trivial
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			)
		{
			using boost::mp11::mp_for_each;
			using boost::mp11::mp_empty;

			static_assert(SystemT::has_trivial_impl);

			using excludes_list = typename SystemT::excludes;
			using removes_list = typename SystemT::removes;
			using adds_list = typename SystemT::adds;

			// Check if we are doing a blanket remove for an optimized system
			// implementation.
			constexpr bool is_removes_only =
				mp_empty<excludes_list>::value && 
				mp_empty<adds_list>::value &&
				!mp_empty<removes_list>::value;

			if constexpr(is_removes_only) {
				_execute_system_trivial_removes_only<SystemT, ChildSystemsListT>(
					info,
					parent,
					action
				);
			} else {
				_execute_system_trivial_default<SystemT, ChildSystemsListT>(
					info,
					parent,
					action
				);
			}
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system_user
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			, const void*                       action
			)
		{
			using boost::mp11::mp_for_each;

			static_assert(!SystemT::has_trivial_impl);

			auto view = ecsact_entt_view(
				std::type_identity<SystemT>{},
				info.registry
			);

			std::for_each(view.begin(), view.end(), [&](auto entity) {
				ecsact_system_execution_context ctx{
					.info = info,
					.entity = entity,
					.parent = parent,
					.action = action,
				};

				// Execute the user defined system implementation
				SystemT::dynamic_impl(
					reinterpret_cast<ecsact::detail::system_execution_context*>(&ctx)
				);

				mp_for_each<ChildSystemsListT>([&]<typename SystemPair>(SystemPair) {
					using boost::mp11::mp_first;
					using boost::mp11::mp_second;
					using ChildSystemT = mp_first<SystemPair>;
					using GrandChildSystemsListT = mp_second<SystemPair>;

					_execute_system<ChildSystemT, GrandChildSystemsListT>(
						info,
						&ctx
					);
				});
			});
		}

		template<typename SystemT, typename ChildSystemsListT>
		void _execute_system
			( registry_info&                    info
			, ecsact_system_execution_context*  parent
			)
		{
			if constexpr(is_action<SystemT>()) {
				auto& actions = std::get<std::vector<SystemT>>(info.actions);
				for(const auto& action : std::get<std::vector<SystemT>>(info.actions)) {
					if constexpr(SystemT::has_trivial_impl) {
						_execute_system_trivial<SystemT, ChildSystemsListT>(
							info,
							parent,
							&action
						);
					} else {
						_execute_system_user<SystemT, ChildSystemsListT>(
							info,
							parent,
							&action
						);
					}
				}
				actions.clear();
			} else {
				if constexpr(SystemT::has_trivial_impl) {
					_execute_system_trivial<SystemT, ChildSystemsListT>(
						info,
						parent,
						nullptr
					);
				} else {
					_execute_system_user<SystemT, ChildSystemsListT>(
						info,
						parent,
						nullptr
					);
				}
			}
		}

		void _clear_transients
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				// Transients require no processing, just clear.
				if constexpr(C::transient) {
					info.registry.clear<C>();
				}
			});
		}

		void _sort_components
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				if constexpr(!std::is_empty_v<C> && !C::transient) {
					// Sorting for deterministic order of components when executing
					// systems.
					// TODO(zaucy): This sort is only necessary for components part of a 
					//              system execution hierarchy greater than 1.
					info.registry.sort<C>([](const C& a, const C& b) -> bool {
						return a < b;
					});
				}
			});
		}

		void _trigger_add_component_events
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				using namespace ::entt::literals;

				::entt::basic_view added_view{
					info.registry.storage<C>(),
					info.registry.storage<component_added<C>>(),
				};
				
				for(entt_entity_type entity : added_view) {
					if constexpr(std::is_empty_v<C>) {
						_invoke_add_callbacks<C>(info, info.ecsact_entity_id(entity), C{});
					} else {
						_invoke_add_callbacks<C>(
							info,
							info.ecsact_entity_id(entity),
							added_view.get<C>(entity)
						);
					}
				}
				
				info.registry.clear<component_added<C>>();
			});
		}

		void _begin_component_change_checks
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::system_writables>([&]<typename C>(C) {
				using namespace ::entt::literals;

				for(auto&& [entity, c] : info.registry.view<C>().each()) {
					std::fprintf(
						stderr,
						"begin system_writables C=%s Entity=%i\n",
						typeid(C).name(),
						entity
					);
					info.registry.storage<C>("beforechange"_hs).emplace(entity, c);
				}
			});
		}

		void _end_component_change_checks
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::system_writables>([&]<typename C>(C) {
				using namespace ::entt::literals;

				::entt::basic_view view{
					info.registry.storage<C>("beforechange"_hs),
					info.registry.storage<C>()
				};
				
				for(auto&& [entity, before, after] : view.each()) {
					std::fprintf(
						stderr,
						"end system_writables C=%s Entity=%i\n",
						typeid(C).name(),
						entity
					);
					if(before != after) {
						_invoke_update_callbacks(
							info,
							info.ecsact_entity_id(entity),
							after
						);
					}
				}

				info.registry.storage<C>("beforechange"_hs).clear();
			});
		}

		void _trigger_remove_component_events
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(C) {
				using namespace ::entt::literals;

				::entt::basic_view before_removed_view{
					info.registry.storage<C>("temp"_hs),
					info.registry.storage<component_removed<C>>(),
				};

				for(entt_entity_type entity : before_removed_view) {
					_invoke_before_remove_callbacks<C>(info, entity, before_removed_view);
					info.registry.storage<C>("temp"_hs).remove(entity);
				}

				::entt::basic_view after_removed_view{
					info.registry.storage<component_removed<C>>(),
				};

				for(entt_entity_type entity : after_removed_view) {
					_invoke_after_remove_callbacks<C>(
						info,
						info.ecsact_entity_id(entity)
					);
				}

				info.registry.clear<component_removed<C>>();
			});
		}

		void _execute_systems
			( registry_info& info
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::execution_order>(
				[&]<typename SystemList>(SystemList) {
					mp_for_each<SystemList>([&]<typename SystemPair>(SystemPair) {
						using boost::mp11::mp_first;
						using boost::mp11::mp_second;

						using SystemT = mp_first<SystemPair>;
						using ChildSystemsListT = mp_second<SystemPair>;
						_execute_system<SystemT, ChildSystemsListT>(
							info,
							nullptr
						);
					});
				}
			);
		}

	public:
		void execute_systems
			( ::ecsact::registry_id reg_id
			)
		{
			using boost::mp11::mp_for_each;

			std::mutex mutex;
			auto& info = _registries.at(reg_id);
			info.mutex = std::ref(mutex);

			_sort_components(info);
			_begin_component_change_checks(info);
			_execute_systems(info);
			_clear_transients(info);
			_trigger_add_component_events(info);
			_end_component_change_checks(info);
			_trigger_remove_component_events(info);

			info.mutex = std::nullopt;
		}

	};

}
