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
auto bytes_hash( //
	std::byte* data,
	int        data_length
) -> std::uint64_t;

template<typename... Args>
auto hash_vals(Args&&... args) -> std::uint64_t {
	auto bytes = bytes_copy(args...);
	return bytes_hash(bytes.data(), bytes.size());
}
} // namespace ecsact::entt::detail
