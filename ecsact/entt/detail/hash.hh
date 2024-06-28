#pragma once

#include "ecsact/entt/detail/bytes.hh"
#include <cstdint>
#include <cstddef>

namespace ecsact::entt::detail {
/**
 * Opaque hash algorithm used by the Ecsact EnTT Runtime
 *
 * @param data bytes to hash
 * @param data_length length of @p data
 * @returns hash value
 */
auto bytes_hash32( //
	std::byte* data,
	int        data_length
) -> std::uint32_t;

auto bytes_hash64( //
	std::byte* data,
	int        data_length
) -> std::uint64_t;

template<typename... Args>
auto hash_vals32(Args&&... args) -> std::uint32_t {
	auto bytes = bytes_copy(args...);
	return bytes_hash32(bytes.data(), bytes.size());
}

template<typename... Args>
auto hash_vals64(Args&&... args) -> std::uint64_t {
	auto bytes = bytes_copy(args...);
	return bytes_hash64(bytes.data(), bytes.size());
}
} // namespace ecsact::entt::detail
