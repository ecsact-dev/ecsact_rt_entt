#pragma once

#include <concepts>
#include <array>
#include <cstddef>

namespace ecsact::entt::detail {

template<std::integral T>
constexpr auto bytes_sizeof() -> int {
	using value_type = std::remove_cvref_t<T>;

	if constexpr(std::is_same_v<value_type, bool>) {
		return 1;
	} else {
		return sizeof(value_type);
	}
}

template<std::integral T>
auto bytes_copy_into( //
	T     v,
	auto& out_bytes,
	auto& out_bytes_offset = 0
) -> void {
	using value_type = std::remove_cvref_t<T>;
	constexpr auto value_size = bytes_sizeof<T>();

	if constexpr(std::is_same_v<value_type, bool>) {
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v ? 1 : 0);
	} else if constexpr(value_size == 1) {
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v);
	} else if constexpr(value_size == 2) {
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 8) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v & 0xFF);
	} else if constexpr(value_size == 4) {
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 24) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 16) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 8) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v & 0xFF);
	} else if constexpr(value_size == 8) {
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 56) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 48) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 40) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 32) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 24) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 16) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>((v >> 8) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v & 0xFF);
	}

	static_assert(value_size <= 8);
}

template<std::integral... T>
auto bytes_copy( //
	T... values
) -> std::array<std::byte, (0 + ... + bytes_sizeof<T>())> {
	auto result = std::array<std::byte, (0 + ... + bytes_sizeof<T>())>{};
	auto offset = 0;
	(bytes_copy_into(values, result, offset), ...);
	return result;
}

} // namespace ecsact::entt::detail
