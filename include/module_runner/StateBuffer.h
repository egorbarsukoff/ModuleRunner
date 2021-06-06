//
// Created by Егор Барсуков on 15.09.2020.
//

#ifndef ELEMENT_TOOLS_BUFFER_INCLUDE_BUFFER_BUFFER_H_
#define ELEMENT_TOOLS_BUFFER_INCLUDE_BUFFER_BUFFER_H_

#include <memory>
#include <memory_resource>
#include <concepts>

template <typename T>
class Buffer {
	std::shared_ptr<T> value;
	std::pmr::synchronized_pool_resource resource;
	std::pmr::polymorphic_allocator<T> allocator{&resource};
 public:
	Buffer() = default;
	Buffer(const Buffer&) {
		assert(0); // Dummy
	}
	Buffer(Buffer&&)  noexcept = default;

	template<typename TT>
	void put(TT&& v) requires std::same_as<std::decay_t<TT>, T> {
		auto new_shared = std::allocate_shared<T>(allocator, std::forward<TT>(v));
		std::atomic_store_explicit(&value, new_shared, std::memory_order_release);
	}

	std::shared_ptr<const T> get() const {
		return std::atomic_load_explicit(&value, std::memory_order_acquire);
	}
};


#endif //ELEMENT_TOOLS_BUFFER_INCLUDE_BUFFER_BUFFER_H_
