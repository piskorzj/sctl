#include "complex_state_chart.h"

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

  SC instance{off,   off_internal, error,  on,         init,
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

TEST_F(ComplexSC, VoidReturnFromHandle) {
  instance.start();

  EXPECT_CALL(off_internal, handle(::testing::An<const Tick &>()));

  instance.handle(Tick{});
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
