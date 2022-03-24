#pragma once

#include <stdexcept>
#include <type_traits>
#include <string>
#include <boost/mp11.hpp>

#ifndef ECSACT_ENTT_RUNTIME_USER_HEADER
#	error ECSACT_ENTT_RUNTIME_USER_HEADER must be defined
#else
#	include ECSACT_ENTT_RUNTIME_USER_HEADER
#endif

#ifndef ECSACT_ENTT_VIEW_HEADER
#	error ECSACT_ENTT_VIEW_HEADER must be defined
#else
#	include ECSACT_ENTT_VIEW_HEADER
#endif

#include "runtime.hh"

#ifndef ECSACT_ENTT_RUNTIME_PACKAGE
# error ECSACT_ENTT_RUNTIME_PACKAGE Must be defined with the fully qualified \
				meta package struct. 
#endif

namespace ecsact_entt_rt {
	extern ecsact::entt::runtime<ECSACT_ENTT_RUNTIME_PACKAGE> runtime;
}

struct ecsact_system_execution_context {
	using package = typename decltype(ecsact_entt_rt::runtime)::package;
	typename decltype(ecsact_entt_rt::runtime)::registry_info& info;
	typename decltype(ecsact_entt_rt::runtime)::entt_entity_type entity;
	const ecsact_system_execution_context* parent;
	const void* action;

	template<typename C>
	void add
		( const C& component
		)
	{
		using ecsact::entt::component_added;
		using ecsact::entt::component_removed;
		using namespace ::entt::literals;

#ifndef NDEBUG
		{
			const bool already_has_component = info.registry.all_of<C>(entity);
			if(already_has_component) {
				std::string err_msg = "Cannot call ctx.add() multiple times. ";
				err_msg += "Added component: ";
				err_msg += typeid(C).name();
				throw std::runtime_error(err_msg.c_str());
			}
		}
#endif

		if constexpr(std::is_empty_v<C>) {
			info.registry.emplace<C>(entity);
		} else {
			info.registry.emplace<C>(entity, component);
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
		using ecsact::entt::component_changed;
		using namespace ::entt::literals;

#ifndef NDEBUG
		{
			const bool already_has_component = info.registry.all_of<C>(entity);
			if(!already_has_component) {
				std::string err_msg = "Cannot call ctx.remove() multiple times. ";
				err_msg += "Removed component: ";
				err_msg += typeid(C).name();
				throw std::runtime_error(err_msg.c_str());
			}
		}
#endif

		if constexpr(!C::transient) {
			if(info.registry.all_of<component_changed<C>>(entity)) {
				info.registry.remove<component_changed<C>>(entity);
			} else if(info.registry.all_of<component_added<C>>(entity)) {
				info.registry.remove<component_added<C>>(entity);
			}
			if constexpr(!std::is_empty_v<C>) {
				auto& temp_storage = info.registry.storage<C>("temp"_hs);
				
				// Store current value of component for the before_remove event later
				if(temp_storage.contains(entity)) {
					temp_storage.get(entity) = info.registry.get<C>(entity);
				} else {
					temp_storage.emplace(entity, info.registry.get<C>(entity));
				}
			}
		}

		info.registry.remove<C>(entity);

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

	template<typename ComponentT>
	ComponentT& get() {
		return info.registry.get<ComponentT>(entity);
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
