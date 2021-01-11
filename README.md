# SCTL - StateChart Template Library

This is an atempt to create mostly compile time, template based state chart library. Also it served as an excuse to use C++20 concepts for the first time for me.

## How it works

Active state can only be non-composite state, if there is transition going to composite-state, it will follow through starting state chain until non-composite state is found.
During transition, first exit handlers are called until common parent state is reached, then enter handlers until destinated state is reached.
Transition can be interrupted by enter handlers if they return state type. Then, new transition is started immediately.

### State classes

Empty object is still valid state, all properties are optional and are used to model state chart directly in code.
Return type from method indicate the transition that will be performed to that type.
Possible return types are:

* `void` - no transition
* `sctl::KeepState` - no transition
* `sctl::State<T>` - transition to T state
* `std::variant<sctl::State<Ts>..., sctl::KeepState>` - transition to state selected in variant (any combination of `sctl::State<T>` is valid, also `stcl::KeepState`), usable for runtime decision

Methods of State class relevant to state chart (`RET` is one of above types):

* `RET enter()` - will be called when state is entered
* `void exit()` - will be called when state is exited
* `RET handle(const Event &)` - will be called when StateChart is handling `Event` and current state is active or is in active state parents chain and more specific states ignored this Event

Type definitions of State class relevant to state chart:

* `using ParentState = T;` - indicates that this state is sub state of `T` state
* `using StartState = T;` - indicates that this state has starting state which is `T`, also that means this state is composite state as it has inner states

### State chart

This object maintains current state and dispatches actions to state objects through references it holds.

Type definition is list of states that are used in state chart.

Constructor - references to states in order.

Methods:

* `void start(bool call_entry = false)` - sets internal state to initial one (first state in list, if composite state, it will be resolved). If call_entry is set, entry chain calls will be done during this call. More than one entry call can happen if first state in list is composite state.
* `void handle(const Event &)` - dispatch event to states, active state handle method will be called, if returns type, transition will be made. If active state does not implements handler, parent state is checked, with same rules. This resolution ends either will matching handler method or hitting last parent in chain

### Events

Event can be any object, it is passed by `const &` to handler function. Dispatching inside StateChart is based only on type of this object and states handler methods. Inside state handler method however, content of the object can be examined and runtime decision on transition can be made - by returing different states.

## Example usage

This is an example how to represent following State Chart in code using this library.

```
@startuml
hide empty description

state On {
  [*] --> Init
  Init --> Ready : Initialized
  Ready --> Busy : Action
  Busy --> Ready : Timeout
  Busy --> Busy : Tick
  Ready --> Config : Configure

  state Config {
    [*] --> Processing
    Processing --> Waiting
    Waiting --> Ready : Timeout
  }

}

state Off {
  [*] --> OffInternal
}

[*] --> Off
Off --> On : PowerOn
On --> Off : PowerOff
On --> Error : Failure
Error --> Off
@enduml
```

```
struct Off;
struct OffInternal;
struct Error;
struct On;
struct Init;
struct Ready;
struct Busy;
struct Config;
struct Processing;
struct Waiting;

struct PowerOn {};
struct PowerOff {};
struct Initialized {};
struct Failure {};
struct Action {};
struct Timeout {};
struct Configure {};
struct Tick {};

struct Off {
  using StartState = OffInternal;
  auto handle(const PowerOn &) { return sctl::State<On>{}; }
};
struct OffInternal {
  using ParentState = Off;
};
struct On {
  using StartState = Init;
  auto handle(const PowerOff &) { return sctl::State<Off>{}; }
  auto handle(const Failure &) { return sctl::State<Error>{}; }
};
struct Error {
  auto enter() {
    StateBase::enter();
    return sctl::State<Off>{};
  }
};
struct Init {
  using ParentState = On;
  auto handle(const Initialized &) { return sctl::State<Ready>{}; }
};
struct Ready {
  using ParentState = On;
  auto handle(const Action &) { return sctl::State<Busy>{}; }
  auto handle(const Configure &) { return sctl::State<Config>{}; }
};
struct Busy {
  using ParentState = On;
  auto handle(const Timeout &) { return sctl::State<Ready>{}; }
  auto handle(const Tick &) { return sctl::State<Busy>{}; }
};
struct Config {
  using ParentState = On;
  using StartState = Processing;
};
struct Processing {
  using ParentState = Config;
  auto enter() {
    StateBase::enter();
    return sctl::State<Waiting>{};
  }
};
struct Waiting {
  using ParentState = Config;
  auto handle(const Timeout &) { return sctl::State<Ready>{}; }
};
```

```
  Off off;
  OffInternal off_internal;
  Error error;
  On on;
  Init init;
  Ready ready;
  Busy busy;
  Config config;
  Processing processing;
  Waiting waiting;

  sctl::StateChart<Off, OffInternal, Error, On, Init, Ready, Busy, Config,
                   Processing, Waiting>
      instance{off,   off_internal, error,  on,         init,
               ready, busy,         config, processing, waiting};
```

```
instance.start();
instance.handle(PowerOn{});
```
