//
// Created by egor on 6/13/19.
//

#ifndef ELEMENT_STATEPROVIDER_H
#define ELEMENT_STATEPROVIDER_H

#include <memory>
#include <any>
#include <unordered_map>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <chrono>
#include <mutex>
#include <set>
#include <concepts>

#include <boost/type_index.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include <condition_variable>

#include <ostream>

#include <iostream>
#include "State.h"

#include "StateBuffer.h"
#include "typeindex"

namespace hana = boost::hana;

class StateProvider {
 private:
  template<typename ...Ts> friend
  class ModuleRunner;

  template<typename ...S> friend
  class Module;

  std::shared_ptr<std::unordered_map<std::type_index, std::any /*Buffer of state */ >> states;
  std::shared_ptr<std::pair<std::shared_mutex, std::unordered_map<std::type_index, std::vector<std::function<void()>>>>>
          notifiers;
  std::shared_ptr<boost::asio::thread_pool> notifiers_workers;
  std::set<std::type_index> allowed_push;

  template<typename ... Types>
  void add_allowed_push(hana::tuple<Types...> states_types) {
      hana::for_each(states_types, [this](auto t) {
          allowed_push.insert(std::type_index(typeid(typename decltype(t)::type)));

      });
  }

  template<State T>
  void call_notifiers() {
      boost::asio::post(*notifiers_workers,
                        [this] {
                            auto type = std::type_index(typeid(std::decay_t<T>));
                            std::shared_lock lock{notifiers->first};
                            for (auto& f : notifiers->second[type]) {
                                f();
                            }
                        });
  }

 public:

  StateProvider(std::shared_ptr<std::unordered_map<std::type_index, std::any>> states_,
                std::shared_ptr<std::pair<std::shared_mutex,
                                          std::unordered_map<std::type_index,
                                                             std::vector<std::function<void()>>>>> notifiers_,
                std::shared_ptr<boost::asio::thread_pool> pool_,
                std::set<std::type_index> allowed_push) :
          states(std::move(states_)),
          notifiers{std::move(notifiers_)},
          notifiers_workers{std::move(pool_)},
          allowed_push(std::move(allowed_push)) {}

  StateProvider(StateProvider&& sp) noexcept = default;
  StateProvider(const StateProvider& sp) = default;
  StateProvider& operator=(StateProvider&& sp) noexcept = default;
  StateProvider& operator=(const StateProvider& sp) = default;

  template<State T>
  std::shared_ptr<const T> get() const {
      if (auto state = states->find(std::type_index(typeid(T))); state != states->end()) {
          auto& buffer = std::any_cast<Buffer<T>&>(state->second);
          return buffer.get();
      }
      return {};
  }

  void push(State auto&& val, [[maybe_unused]] bool with_notify = true) {
      using T = std::decay_t<decltype(val)>;
      auto type = std::type_index(typeid(T));
      std::any_cast<Buffer<T>&>(states->at(type)).put(std::forward<decltype(val)>(val));
      if (with_notify) {
          call_notifiers<T>();
      }
  }

  template<State T>
  void subscribe_notifier(std::function<void()> f) {
      std::unique_lock lock{notifiers->first};
      notifiers->second[std::type_index(typeid(std::decay_t<T>))].push_back(std::move(f));
  }

};

#endif //ELEMENT_STATEPROVIDER_H
