#pragma once

#include <cstdint>
#include <type_traits>

namespace ecsact::entt::detail {

template<typename C, std::size_t FieldOffset>
struct association {
	using component = C;
	static constexpr auto field_offset = FieldOffset;
};

template<typename C>
struct temp_storage;

template<typename C>
	requires(std::is_empty_v<C>)
struct temp_storage<C> {};

template<typename C>
	requires(!std::is_empty_v<C>)
struct temp_storage<C> {
	C value;
};

template<typename C>
	requires(!std::is_empty_v<C>)
struct beforechange_storage {
	C    value;
	bool set = false;
};

template<typename C>
struct pending_add;

template<typename C>
	requires(std::is_empty_v<C>)
struct pending_add<C> {};

template<typename C>
	requires(!std::is_empty_v<C>)
struct pending_add<C> {
	C value;
};

template<typename C>
struct pending_remove {};

} // namespace ecsact::entt::detail
