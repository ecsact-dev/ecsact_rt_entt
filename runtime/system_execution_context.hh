#pragma once

#include <stdexcept>
#include <type_traits>
#include <string>
#include <unordered_set>
#include <boost/mp11.hpp>
#include <entt/entt.hpp>

#include "registry_info.hh"
#include "event_markers.hh"

namespace ecsact_entt_rt {
	template<typename Package>
	struct system_execution_context {
		using cptr_t = struct ::ecsact_system_execution_context*;
		using const_cptr_t = const struct ::ecsact_system_execution_context*;
		using cpp_ptr_t = ecsact::detail::system_execution_context*;
		using const_cpp_ptr_t = ecsact::detail::system_execution_context*;

		using package = Package;
		ecsact_entt_rt::registry_info<Package>& info;
		::entt::entity entity;
		const cptr_t parent;
		const void* action;
		std::unordered_set<::ecsact::component_id> writables;

		/**
		 * Pointer for ecsact C system execution
		 */
		inline cptr_t cptr() noexcept {
			return reinterpret_cast<cptr_t>(this);
		}

		/**
		 * Pointer for ecsact C system execution
		 */
		inline const_cptr_t cptr() const noexcept {
			return reinterpret_cast<const_cptr_t>(this);
		}

		/**
		 * Pointer for ecsact C++ system execution
		 */
		inline cpp_ptr_t cpp_ptr() noexcept {
			return reinterpret_cast<cpp_ptr_t>(this);
		}

		/**
		 * Pointer for ecsact C++ system execution
		 */
		inline const_cpp_ptr_t cpp_ptr() const noexcept {
			return reinterpret_cast<const_cpp_ptr_t>(this);
		}

		template<typename C>
		void add
			( const C& component
			)
		{
			using ecsact::entt::component_added;
			using ecsact::entt::component_removed;
			using ecsact::entt::detail::pending_add;
			using namespace std::string_literals;

#ifndef NDEBUG
			{
				const bool already_has_component =
					info.registry.all_of<pending_add<C>>(entity);
				if(already_has_component) {
					std::string err_msg = "Cannot call ctx.add() multiple times. ";
					err_msg += "Added component: "s + typeid(C).name();
					throw std::runtime_error(err_msg.c_str());
				}
			}
#endif

			if constexpr(std::is_empty_v<C>) {
				info.registry.emplace<pending_add<C>>(entity);
			} else {
				info.registry.emplace<pending_add<C>>(entity, component);
			}

			if constexpr(!C::transient) {
				if(info.registry.all_of<component_removed<C>>(entity)) {
					info.registry.remove<component_removed<C>>(entity);
				} else {
					info.registry.emplace<component_added<C>>(entity);
				}
			}
		}

		void add
			( ::ecsact::component_id  component_id
			, const void*             component_data
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					add<C>(*static_cast<const C*>(component_data));
				}
			});
		}

		template<typename C>
		void remove() {
			using ecsact::entt::component_removed;
			using ecsact::entt::component_added;
			using ecsact::entt::detail::temp_storage;
			using ecsact::entt::detail::pending_remove;

#ifndef NDEBUG
			[[maybe_unused]] auto component_name = typeid(C).name();

			{
				const bool already_has_component = info.registry.all_of<C>(entity);
				if(!already_has_component) {
					std::string err_msg = "Cannot call ctx.remove() multiple times. ";
					err_msg += "Removed component: ";
					err_msg += component_name;
					throw std::runtime_error(err_msg.c_str());
				}
			}
#endif

			if constexpr(!C::transient) {
				if(info.registry.all_of<component_added<C>>(entity)) {
					info.registry.remove<component_added<C>>(entity);
				}
				if constexpr(!std::is_empty_v<C>) {
					auto& temp = info.registry.storage<temp_storage<C>>();

					// Store current value of component for the before_remove event later
					if(temp.contains(entity)) {
						temp.get(entity).value = info.registry.get<C>(entity);
					} else {
						temp.emplace(entity, info.registry.get<C>(entity));
					}
				}
			}

			info.registry.emplace<pending_remove<C>>(entity);

			if constexpr(!C::transient) {
				info.registry.emplace<component_removed<C>>(entity);
			}
		}

		void remove
			( ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					remove<C>();
				}
			});
		}

		template<typename C>
		C& get() {
			using ecsact::entt::detail::beforechange_storage;
			using ecsact::entt::component_changed;

			C& comp = info.registry.get<C>(entity);

			if(writables.contains(C::id)) {
				const bool should_store_beforechange = !info.registry.any_of<
					component_changed<C>,
					beforechange_storage<C>
				>(entity);
				if(should_store_beforechange) {
					info.registry.emplace<beforechange_storage<C>>(entity, comp);
				}
			}

			return comp;
		}

		void* get
			( ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			void* component = nullptr;
			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					if constexpr(!std::is_empty_v<C>) {
						component = &(get<C>());
					}
				}
			});
			return component;
		}

		template<typename ComponentT>
		bool has() {
			return info.registry.all_of<ComponentT>(entity);
		}

		bool has
			( ::ecsact::component_id  component_id
			)
		{
			using boost::mp11::mp_for_each;

			bool result = false;
			mp_for_each<typename package::components>([&]<typename C>(const C&) {
				if(C::id == component_id) {
					result = has<C>();
				}
			});
			return result;
		}

		void generate
			( int                      component_count
			, ::ecsact::component_id*  component_ids
			, const void**             components_data
			)
		{
			using boost::mp11::mp_for_each;
			using ecsact::entt::component_added;

			auto new_entity = info.create_entity().entt_entity_id;
			for(int i=0; component_count > i; ++i) {
				auto component_id = component_ids[i];
				auto component_data = components_data[i];
				mp_for_each<typename package::components>([&]<typename C>(const C&) {
					if(C::id == component_id) {
						if constexpr(std::is_empty_v<C>) {
							info.registry.emplace<C>(new_entity);
						} else {
							info.registry.emplace<C>(
								new_entity,
								*static_cast<const C*>(component_data)
							);
						}

						info.registry.emplace<component_added<C>>(entity);
					}
				});
			}
		}
	};
}