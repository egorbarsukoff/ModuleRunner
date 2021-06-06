#include "module_runner/State.h"

uint64_t BaseState::id_counter = 0;

BaseState::BaseState() : time(std::chrono::system_clock::now()), id(++id_counter) {
}

std::ostream &operator<<(std::ostream &os, const BaseState &state) {
    os << "id: " << state.id;
    return os;
}

void BaseState::update_id() {
    id = ++id_counter;
    time = std::chrono::system_clock::now();
}