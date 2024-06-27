#include "ecsact/entt/detail/hash.hh"
#include "xxhash.h"

auto ecsact::entt::detail::bytes_hash32( //
	std::byte* data,
	int        data_length
) -> std::uint32_t {
	if(data_length == sizeof(std::uint32_t)) {
		auto bytes_as_u32 = std::uint32_t{};
		std::memcpy(&bytes_as_u32, data, sizeof(std::uint32_t));
		return bytes_as_u32;
	} else if(data_length == sizeof(std::uint16_t)) {
		auto bytes_as_u16 = std::uint16_t{};
		std::memcpy(&bytes_as_u16, data, sizeof(std::uint16_t));
		return bytes_as_u16;
	} else if(data_length == 1) {
		return static_cast<std::uint64_t>(data[0]);
	} else if(data_length == 0) {
		return 0;
	}

	auto hash = XXH32(data, data_length, 1);
	return hash;
}

auto ecsact::entt::detail::bytes_hash64( //
	std::byte* data,
	int        data_length
) -> std::uint64_t {
	if(data_length == sizeof(std::uint64_t)) {
		auto bytes_as_u64 = std::uint64_t{};
		std::memcpy(&bytes_as_u64, data, sizeof(std::uint64_t));
		return bytes_as_u64;
	} else if(data_length == sizeof(std::uint32_t)) {
		auto bytes_as_u32 = std::uint32_t{};
		std::memcpy(&bytes_as_u32, data, sizeof(std::uint32_t));
		return bytes_as_u32;
	} else if(data_length == sizeof(std::uint16_t)) {
		auto bytes_as_u16 = std::uint16_t{};
		std::memcpy(&bytes_as_u16, data, sizeof(std::uint16_t));
		return bytes_as_u16;
	} else if(data_length == 1) {
		return static_cast<std::uint64_t>(data[0]);
	} else if(data_length == 0) {
		return 0;
	}

	XXH64_hash_t hash = XXH3_64bits(data, data_length);
	return hash;
}
