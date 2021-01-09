#include "gmock/gmock.h"

#include "sctl.h"

/*
 Simple FSM

 on state, off state, turn-on and turn-off actions

*/

struct ActionTurnOn {};
struct ActionTurnOff {};

struct StateOn;
struct StateOff;

struct StateOn {
  MOCK_METHOD(void, enter, ());
  MOCK_METHOD(void, exit, ());

  auto handle(const ActionTurnOff &) { return sctl::State<StateOff>{}; }
};

struct StateOff {
  MOCK_METHOD(void, enter, ());
  MOCK_METHOD(void, exit, ());

  auto handle(const ActionTurnOn &a) { return sctl::State<StateOn>{}; }
};

struct SimpleFSM : public ::testing::Test {
  StateOn on;
  StateOff off;

  sctl::StateChart<StateOff, StateOn> instance{off, on};
};

TEST_F(SimpleFSM, NoEffectWhenNotStarted) {
  EXPECT_CALL(off, exit()).Times(0);

  instance.handle(ActionTurnOn{});
}

TEST_F(SimpleFSM, CallEntryMethodWhenStartedWithEntry) {
  EXPECT_CALL(off, enter());

  instance.start(true);
}

TEST_F(SimpleFSM, BasicTransition) {

  instance.start();

  EXPECT_CALL(off, exit());
  EXPECT_CALL(on, enter());

  instance.handle(ActionTurnOn{});

  EXPECT_CALL(on, exit());
  EXPECT_CALL(off, enter());

  instance.handle(ActionTurnOff{});
}
