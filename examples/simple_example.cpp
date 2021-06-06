
#include <module_runner/ModuleRunner.h>
#include <module_runner/Module.h>
#include <module_runner/State.h>

#include <utility>

struct State1 : BaseState {
  State1(std::string  value) : value(std::move(value)) {}
  std::string value;
};

struct State2 : BaseState {
  State2(const std::string& value) : value(value) {}
  std::string value;
};

class Module1 : public Module<State1> {
 public:
  explicit Module1(StateProvider sp) : Module<State1>(std::move(sp), 1) {}
  void loop_iteration() {
      sp.push(State1{std::to_string(std::chrono::system_clock::now().time_since_epoch().count())});
  }
};

class Module2 : public Module<State2> {
 public:
  explicit Module2(StateProvider sp) : Module<State2>(std::move(sp)) {
      this->sp.subscribe_notifier<State1>([this] {
         auto state1 = this->sp.get<State1>();
         this->sp.push(State2{
             "got value: " + state1->value + " with id " + std::to_string(state1->id)
         });
      });
  }
};

int main() {
    ModuleRunner<Module1, Module2> mr;
    auto sp = mr.getSP();
    sp.subscribe_notifier<State2>([&sp]{
        std::cout << "state2: " << sp.get<State2>()->value << std::endl;
    });
    mr.start_all();
    std::this_thread::sleep_for(20s);
}