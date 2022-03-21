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

	template<typename ComponentT>
	void add
		( const ComponentT& component
		)
	{
		using ecsact::entt::component_added;

#ifndef NDEBUG
		{
			const bool already_has_component =
				info.pending_events_registry.all_of<ComponentT>(entity);
			if(already_has_component) {
				std::string err_msg = "Cannot call ctx.add() multiple times. ";
				err_msg += "Added component: ";
				err_msg += typeid(ComponentT).name();
				throw std::runtime_error(err_msg.c_str());
			}
		}
#endif

		if constexpr(std::is_empty_v<ComponentT>) {
			info.registry.emplace<ComponentT>(entity);
		} else {
			info.registry.emplace<ComponentT>(entity, component);
		}

		info.pending_events_registry.emplace<component_added<ComponentT>>(entity);
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

	template<typename ComponentT>
	void remove() {
		using ecsact::entt::component_removed;

#ifndef NDEBUG
		{
			const bool already_has_component =
				info.pending_events_registry.all_of<ComponentT>(entity);
			if(!already_has_component) {
				std::string err_msg = "Cannot call ctx.remove() multiple times. ";
				err_msg += "Removed component: ";
				err_msg += typeid(ComponentT).name();
				throw std::runtime_error(err_msg.c_str());
			}
		}
#endif

		info.registry.remove<ComponentT>(entity);
		info.pending_events_registry.emplace<component_removed<ComponentT>>(entity);
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

					info.pending_events_registry.emplace<component_added<C>>(entity);
				}
			});
		}
	}
};
