#pragma once

#include <cstddef>
#include <array>
#include <memory>
#include <atomic>
#include <type_traits>

namespace ecsact::entt::detail {

template<std::size_t Alignment, std::size_t Size>
struct static_allocator_storage {
	using buffer_t = std::array<std::byte, Size>;

	std::unique_ptr<buffer_t> buffer_ = std::make_unique<buffer_t>();
	std::atomic_size_t        offset_ = 0;
};

struct static_allocator_base {
	static constexpr std::size_t static_size = 1e+8;

	static static_allocator_storage<1, static_size> storage1_;
	static static_allocator_storage<4, static_size> storage4_;
	static static_allocator_storage<8, static_size> storage8_;
};

template<typename T>
struct static_allocator : static_allocator_base {
	using value_type = T;

	static_allocator() = default;
	static_allocator(const static_allocator& other) = default;

	static_allocator(static_allocator&& other) = default;

	template<typename U>
	static_allocator(const static_allocator<U>&) {
	}

	template<typename U>
	static_allocator(static_allocator<U>&&) {
	}

	[[nodiscard]] auto allocate(std::size_t n) noexcept -> T* {
		constexpr auto alignment = std::alignment_of_v<T>;

		if constexpr(alignment == 1) {
			std::size_t alloc_start = storage1_.offset_.fetch_add(sizeof(T) * n);
			return std::assume_aligned<alignment>(
				reinterpret_cast<T*>(storage1_.buffer_->data() + alloc_start)
			);
		}

		if constexpr(alignment == 4) {
			std::size_t alloc_start = storage4_.offset_.fetch_add(sizeof(T) * n);
			return std::assume_aligned<alignment>(
				reinterpret_cast<T*>(storage4_.buffer_->data() + alloc_start)
			);
		}

		if constexpr(alignment == 8) {
			std::size_t alloc_start = storage8_.offset_.fetch_add(sizeof(T) * n);
			return std::assume_aligned<alignment>(
				reinterpret_cast<T*>(storage8_.buffer_->data() + alloc_start)
			);
		}

		static_assert(
			alignment == 1 || alignment == 4 || alignment == 8,
			"Missing support for alignment"
		);
	}

	auto deallocate(T* p, std::size_t n) noexcept -> void {
	}
};

template<typename T, typename U>
auto operator==(const static_allocator<T>&, const static_allocator<U>&)
	-> bool {
	return true;
}

template<typename T, typename U>
auto operator!=(const static_allocator<T>&, const static_allocator<U>&)
	-> bool {
	return false;
}

} // namespace ecsact::entt::detail
