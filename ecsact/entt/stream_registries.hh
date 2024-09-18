#pragma once

#include <thread>
#include <map>
#include <mutex>
#include <vector>
#include <ranges>
#include <memory>
#include <entt/entt.hpp>
#include "ecsact/runtime/common.h"
#include "ecsact/entt/entity.hh"

namespace ecsact::entt::stream::detail {
using child_reg_thread_map =
	std::unordered_map<std::thread::id, std::unique_ptr<::entt::registry>>;
using reg_thread_map =
	std::unordered_map<ecsact_registry_id, child_reg_thread_map>;
} // namespace ecsact::entt::stream::detail

namespace ecsact::entt::stream {
class stream_registries {
private:
	detail::reg_thread_map registry_stream_threads;
	std::mutex             stream_mutex;

public:
	stream_registries() = default;

	template<typename C>
	auto handle_stream(
		ecsact_registry_id registry_id,
		ecsact_entity_id   entity_id,
		const C&           component
	) -> void {
		// Add to map if new threads/registries are introduced

		std::unique_lock lk(stream_mutex);
		auto             thread_id = std::this_thread::get_id();

		auto reg_threads_itr = registry_stream_threads.find(registry_id);
		auto reg_thread_itr = detail::child_reg_thread_map::iterator();

		if(reg_threads_itr == registry_stream_threads.end()) {
			auto thread_map = detail::child_reg_thread_map{};

			reg_thread_itr = thread_map.emplace_hint(
				thread_map.end(),
				std::pair(thread_id, std::make_unique<::entt::registry>())
			);

			reg_threads_itr = registry_stream_threads.insert(
				registry_stream_threads.end(),
				std::pair(registry_id, std::move(thread_map))
			);
		} else {
			auto& reg_threads = reg_threads_itr->second;
			reg_thread_itr = reg_threads.find(thread_id);

			if(reg_thread_itr == reg_threads.end()) {
				reg_thread_itr = reg_threads.insert(
					reg_threads.end(),
					std::pair(thread_id, std::make_unique<::entt::registry>())
				);
			}
		}

		auto& registry = reg_thread_itr->second;

		auto entity = ::ecsact::entt::entity_id(entity_id);

		if(!registry->valid(entity)) {
			auto new_entity = registry->create(entity);
			assert(new_entity == entity.as_entt());
		}

		registry->template emplace_or_replace<C>(entity, component);
	}

	auto get_stream_registries()
		-> std::vector<std::unique_ptr<::entt::registry>>;
};
} // namespace ecsact::entt::stream
