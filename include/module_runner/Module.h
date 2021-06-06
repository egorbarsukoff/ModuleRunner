//
// Created by egor on 6/14/19.
//

#ifndef ELEMENT_MODULE_H
#define ELEMENT_MODULE_H

#include <variant>
#include <boost/hana/tuple.hpp>

namespace hana = boost::hana;

#include "StateProvider.h"
#include "State.h"
#include <atomic>

template<typename ...S>
class Module {
 protected:
  static_assert((is_state<S> && ...), "S must be states");

  std::atomic<bool> configured = true;
  StateProvider sp;
 public:
  static constexpr hana::tuple<hana::type < S>...>
  states = hana::tuple_t<S...>;
  const std::chrono::system_clock::duration period;
  Module() = delete;
  template<typename ...Ts> friend
  class ModuleRunner;

  explicit Module(StateProvider sp_, unsigned freq = -1u) : sp(std::move(sp_)),
                                                            period{std::chrono::nanoseconds{
                                                                    static_cast<unsigned>(freq == -1u ? 0. :
                                                                                          1. / freq * 1e9)}} {
      sp.add_allowed_push(states);
  }
};

#endif //ELEMENT_MODULE_H
