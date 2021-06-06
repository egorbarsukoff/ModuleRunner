//
// Created by egor on 7/20/19.
//

#ifndef ELEMENT_STATE_H
#define ELEMENT_STATE_H

#include <chrono>
#include <cctype>
#include <iostream>
#include <variant>
#include <functional>

struct BaseState {
 private:
	static uint64_t id_counter;
 public:
	std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
	uint64_t id;
	BaseState();
	BaseState(const BaseState &st) = default;
	BaseState(BaseState &&st) noexcept = default;
	BaseState &operator=(const BaseState &st) = default;
	BaseState &operator=(BaseState &&st) = default;
	void update_id();
 protected:
	friend std::ostream &operator<<(std::ostream &os, const BaseState &state);
};

template<typename T>
constexpr bool is_state = std::is_base_of_v<BaseState, T>;

template<typename T>
concept State = std::is_base_of_v<BaseState, std::decay_t<T>>;


#endif //ELEMENT_STATE_H
