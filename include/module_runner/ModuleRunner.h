//
// Created by egor on 16.01.19.
//

#ifndef ELEMENT_MODULERUNNER_H
#define ELEMENT_MODULERUNNER_H

#include <thread>
#include <atomic>
#include <type_traits>
#include <tuple>
#include <any>
#include <map>
#include <boost/hana/tuple.hpp>
#include <boost/hana/unique.hpp>
#include <condition_variable>
#include <future>
#include <utility>
#include <typeindex>

#include "module_runner/StateProvider.h"
#include "module_runner/Module.h"

namespace hana = boost::hana;
using namespace std::chrono_literals;

template<typename... Ts>
class ModuleRunner {

	static_assert(hana::unique(hana::make_tuple(hana::type_c<Ts>...)) == hana::make_tuple(hana::type_c<Ts>...),
								"Modules must be unique");

	template<class T, class = void>
	class has_unblock_t : public std::false_type {};

	template<class T>
	class has_unblock_t<T, std::void_t<decltype(&T::unblock)>> : public std::true_type {};

	template<class T>
	static constexpr bool has_unblock = has_unblock_t<T>{};

	StateProvider sp;
	std::tuple<std::pair<std::unique_ptr<Ts>, std::atomic<bool>>...> modules;
	std::tuple<std::pair<hana::type<Ts>, std::promise<void>>...> last_promises;
	std::tuple<std::pair<hana::type<Ts>, std::optional<std::thread>>...> threads;

	template<typename T>
	void loop() {
		using namespace std::chrono;
		using Module = std::decay_t<decltype(*T())>;
		if (!std::get<std::pair<T, std::atomic<bool>>>(modules).first->configured.load(std::memory_order_acquire)) {
			throw std::runtime_error("Module is not configured");
		}
		if constexpr (requires { module<Module>().loop_iteration(); }) {
			while (module_status<Module>().load(std::memory_order_acquire)) {
				auto start = system_clock::now();
				module<Module>().loop_iteration();
				if (auto period = system_clock::now() - start; period < module<Module>().period) {
					std::this_thread::sleep_until(start + module<Module>().period);
				}
			}
		}
		last_promise<Module>().set_value();
	}

	template<typename T>
	std::atomic<bool> &module_status() {
		return std::get<std::pair<std::unique_ptr<T>, std::atomic<bool>>>(modules).second;
	}

	template<typename T>
	std::promise<void> &last_promise() {
		return std::get<std::pair<hana::type<T>, std::promise<void>>>(last_promises).second;
	}

	template<typename T>
	std::optional<std::thread> &module_thread() {
		return std::get<std::pair<hana::type<T>, std::optional<std::thread>>>(threads).second;
	}

	static StateProvider create_sp() {
	  auto sp = StateProvider{std::make_shared<std::unordered_map<std::type_index, std::any>>(),
          std::make_shared<std::pair<std::shared_mutex, std::unordered_map<std::type_index, std::vector<std::function<void()>>>>>(),
          std::make_shared<boost::asio::thread_pool>(4),
          std::set<std::type_index>()};

      hana::for_each(hana::make_tuple(Ts::states...), [&](auto t) {
        hana::for_each(t, [&](auto s) {
          using T = typename decltype(s)::type;
          std::any v;
          v.emplace<Buffer<T>>();
          sp.states->insert(std::make_pair<std::type_index, std::any>(
              std::type_index(typeid(T)),
              std::move(v)));
        });
      });

      return sp;
	}

 public:
	ModuleRunner() : sp{create_sp()}, modules(std::make_pair(std::make_unique<Ts>(sp), false)...) {}

	template<typename T>
	void start() {
		if (!module_status<T>().load(std::memory_order_acquire)) {
			module_status<T>().store(true, std::memory_order_release);
			auto m_thread = std::thread(
					&ModuleRunner<Ts...>::loop<std::decay_t<decltype(std::get<std::pair<std::unique_ptr<T>,
																																							std::atomic<bool>>>(modules)
							.first)>>, this);
			module_thread<T>() = std::move(m_thread);
		}
	}

	void start_all(const std::chrono::seconds &timeout = 0s) {
		hana::for_each(hana::make_tuple(hana::type_c<Ts>...), [this, &timeout](auto t) {
			this->start<typename decltype(t)::type>();
			std::this_thread::sleep_for(timeout);
		});
	}

	template<typename T>
	void stop() {
		if (module_status<T>().load(std::memory_order_acquire)) {
			module_status<T>().store(false, std::memory_order_release);
			if constexpr (has_unblock<T>) {
				module<T>().unblock();
			}
			bool ends = (last_promise<T>().get_future().wait_for(std::chrono::milliseconds(1000))
					!= std::future_status::timeout);
			if (!ends) {
				auto handle = module_thread<T>()->native_handle();
				module_thread<T>()->detach();
				module_thread<T>() = std::nullopt;
				pthread_cancel(handle);
			}
		}
		if (module_thread<T>() && module_thread<T>()->joinable()) {
			module_thread<T>()->join(); // barrier already reached!
			module_thread<T>() = std::nullopt;
		}
		last_promise<T>() = std::promise<void>{};
	}

	template<typename T>
	T &module() {
		return *std::get<std::pair<std::unique_ptr<T>, std::atomic<bool>>>(modules).first;
	}

	StateProvider getSP() {
		return sp;
	}

	~ModuleRunner() {
		hana::for_each(hana::make_tuple(hana::type_c<Ts>...), [this](auto t) {
			using Module = typename decltype(t)::type;
			this->stop<Module>();
		});
	}
};

#endif //ELEMENT_MODULERUNNER_H
