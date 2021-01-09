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
  auto enter() {
    StateBase::enter();
    return sctl::State<Waiting>{};
  }
};
struct Waiting : StateBase {
  using ParentState = Config;
  auto handle(const Timeout &) { return sctl::State<Ready>{}; }
};

struct ComplexSC : public ::testing::Test {
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
};

TEST_F(ComplexSC, UsingStartingStateAndParentState) {
  ::testing::InSequence seq;

  instance.start();

  EXPECT_CALL(off_internal, exit());
  EXPECT_CALL(off, exit());
  EXPECT_CALL(on, enter());
  EXPECT_CALL(init, enter());

  instance.handle(PowerOn{});
}

TEST_F(ComplexSC, CallEntryMethodWhenStartedWithEntry) {
  EXPECT_CALL(off, enter());
  EXPECT_CALL(off_internal, enter());

  instance.start(true);
}

TEST_F(ComplexSC, HandleEventOnParentState) {
  ::testing::InSequence seq;

  instance.start();

  EXPECT_CALL(off_internal, exit());
  EXPECT_CALL(off, exit());
  EXPECT_CALL(on, enter());
  EXPECT_CALL(init, enter());

  instance.handle(PowerOn{});

  EXPECT_CALL(init, exit());
  EXPECT_CALL(ready, enter());

  instance.handle(Initialized{});

  EXPECT_CALL(ready, exit());
  EXPECT_CALL(config, enter());
  EXPECT_CALL(processing, enter());
  EXPECT_CALL(processing, exit());
  EXPECT_CALL(waiting, enter());

  instance.handle(Configure{});

  EXPECT_CALL(waiting, exit());
  EXPECT_CALL(config, exit());
  EXPECT_CALL(on, exit());
  EXPECT_CALL(off, enter());
  EXPECT_CALL(off_internal, enter());

  instance.handle(PowerOff{});
}

TEST_F(ComplexSC, SelfAction) {
  ::testing::InSequence seq;

  instance.start();

  EXPECT_CALL(off_internal, exit());
  EXPECT_CALL(off, exit());
  EXPECT_CALL(on, enter());
  EXPECT_CALL(init, enter());

  instance.handle(PowerOn{});

  EXPECT_CALL(init, exit());
  EXPECT_CALL(ready, enter());

  instance.handle(Initialized{});

  EXPECT_CALL(ready, exit());
  EXPECT_CALL(busy, enter());

  instance.handle(Action{});

  EXPECT_CALL(busy, exit());
  EXPECT_CALL(busy, enter());

  instance.handle(Tick{});
}

TEST_F(ComplexSC, TransitionFromBegin) {
  ::testing::InSequence seq;

  instance.start();

  EXPECT_CALL(off_internal, exit());
  EXPECT_CALL(off, exit());
  EXPECT_CALL(on, enter());
  EXPECT_CALL(init, enter());

  instance.handle(PowerOn{});

  EXPECT_CALL(init, exit());
  EXPECT_CALL(on, exit());
  EXPECT_CALL(error, enter());
  EXPECT_CALL(error, exit());
  EXPECT_CALL(off, enter());
  EXPECT_CALL(off_internal, enter());

  instance.handle(Failure{});
}
