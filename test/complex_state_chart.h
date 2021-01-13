#pragma once

#include "gmock/gmock.h"

#include "sctl.h"

/*
 Complex State Chart

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

*/

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

struct StateBase {
  MOCK_METHOD(void, enter, ());
  MOCK_METHOD(void, exit, ());
};

struct Off : StateBase {
  using StartState = OffInternal;
  auto handle(const PowerOn &) { return sctl::State<On>{}; }
};
struct OffInternal : StateBase {
  using ParentState = Off;
  MOCK_METHOD(void, handle, (const Tick &));
};
struct On : StateBase {
  using StartState = Init;
  auto handle(const PowerOff &) { return sctl::State<Off>{}; }
  auto handle(const Failure &) { return sctl::State<Error>{}; }
};
struct Error : StateBase {
  auto enter() {
    StateBase::enter();
    return sctl::State<Off>{};
  }
};
struct Init : StateBase {
  using ParentState = On;
  auto handle(const Initialized &) { return sctl::State<Ready>{}; }
};
struct Ready : StateBase {
  using ParentState = On;
  auto handle(const Action &) { return sctl::State<Busy>{}; }
  auto handle(const Configure &) { return sctl::State<Config>{}; }
};
struct Busy : StateBase {
  using ParentState = On;
  auto handle(const Timeout &) { return sctl::State<Ready>{}; }
  auto handle(const Tick &) { return sctl::State<Busy>{}; }
};
struct Config : StateBase {
  using ParentState = On;
  using StartState = Processing;
};
struct Processing : StateBase {
  using ParentState = Config;
  std::variant<sctl::State<Waiting>, sctl::KeepState> enter() {
    StateBase::enter();
    return sctl::State<Waiting>{};
  }
};
struct Waiting : StateBase {
  using ParentState = Config;
  std::variant<sctl::State<Ready>, sctl::KeepState> handle(const Timeout &) {
    return sctl::State<Ready>{};
  }
};

using SC = sctl::StateChart<Off, OffInternal, Error, On, Init, Ready, Busy,
                            Config, Processing, Waiting>;
