#include <memory>

#include <entt/entt.hpp>

#include "ecsact/entt/stream_registries.hh"
#include "ecsact/entt/detail/globals.hh"

ecsact::entt::stream::stream_registries
	ecsact::entt::detail::globals::stream_registries = {};

auto ecsact::entt::stream::stream_registries::get_stream_registries()
	-> std::vector<std::unique_ptr<::entt::registry>> {
	std::unique_lock lk(stream_mutex);
	auto registries = std::vector<std::unique_ptr<::entt::registry>>{};

	for(auto&& [reg_id, map] : registry_stream_threads) {
		for(auto&& [thread_id, registry] : map) {
			if(!registry->empty()) {
				registries.push_back(std::move(registry));
				registry = std::make_unique<::entt::registry>();
			} else {
				// TODO(Kelwan): Clean up hanging threads here?
			}
		}
	}
	return registries;
}
