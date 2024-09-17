#pragma once

#include <thread>
#include <map>
#include <mutex> // IWYU pragma: keep
#include <optional>
#include "detail/registry.hh"
#include <entt/entt.hpp>
#include "ecsact/entt/registry_util.hh" // IWYU pragma: keep

namespace ecsact::entt::stream {

using child_reg_thread_map = std::map<std::thread::id, ::entt::registry>;
using reg_thread_map = std::map<ecsact_registry_id, child_reg_thread_map>;

class stream_registries {
public:
	stream_registries();

	template<typename C>
	auto handle_stream(
		ecsact_registry_id registry_id,
		ecsact_entity_id   entity_id,
		const C&           component
	) -> void {
    //Add to map if new threads/registries are introduced
		auto thread_id = std::this_thread::get_id();

    auto reg_threads_itr = registry_stream_threads.find(registry_id);

		if(reg_threads_itr == registry_stream_threads.end()) {
      ::entt::registry registry();

			stream_mutex.lock();
			reg_threads_itr = registry_stream_threads.insert(
				std::pair(
					registry_id,
					child_reg_thread_map{thread_id, registry.create()}
				)
			);
		} else {
      // Registry is in! Now find thread
      auto& reg_threads = reg_threads_itr->second;
      auto reg_thread_itr = reg_threads.find(thread_id);
      if(reg_thread_itr == reg_threads.end()) {
        ::entt::registry registry();
        stream_mutex.lock();
        reg_thread_itr = reg_threads.insert(std::pair(thread_id, registry.create()));
      }
    }

    auto& registry = reg_threads_itr->second->second;

    registry.template emplace_or_replace<C>(entity_id, component);
	}

private:
	reg_thread_map registry_stream_threads;
	std::mutex     stream_mutex;
};
} // namespace ecsact::entt::stream
