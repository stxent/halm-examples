/*
 * generic_default/timer_factory_restarts/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/delay.h>
#include <halm/generic/timer_factory.h>
#include <halm/platform/generic/timer.h>
#include <check.h>
#include <uv.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#define MAX_ITERATIONS   10
#define MAX_TIMERS       4
#define TICKS_PER_QUANTA 11
#define WAIT_TIMEOUT_MS  5000
/*----------------------------------------------------------------------------*/
static const uint32_t testTaskPeriods[] = {TICKS_PER_QUANTA, 17, 29, 41, 401};

struct Context
{
  struct Timer *factory;
  struct Timer *timer;
  uint32_t count;
  uint32_t limit;
  uint32_t period;
  uint32_t timestamps[WAIT_TIMEOUT_MS / TICKS_PER_QUANTA];
  bool autostop;
};
/*----------------------------------------------------------------------------*/
static void checkPeriods(struct Context *);
static void checkVariablePeriods(struct Context *);
static void drainLoop(void);
static bool isTestFinished(void *);
static void periodicTask(void *);
static bool waitForCompletion(bool (*)(void *), void *, uint64_t);
/*----------------------------------------------------------------------------*/
static void checkPeriods(struct Context *context)
{
  for (uint32_t i = 1; i < context->count; ++i)
  {
    ck_assert_uint_eq(context->timestamps[i] - context->timestamps[i - 1],
        context->period);
  }
}
/*----------------------------------------------------------------------------*/
static void checkVariablePeriods(struct Context *context)
{
  for (uint32_t i = 1; i < context->count; ++i)
  {
    ck_assert_uint_eq(context->timestamps[i] - context->timestamps[i - 1],
        testTaskPeriods[i % ARRAY_SIZE(testTaskPeriods)]);
  }
}
/*----------------------------------------------------------------------------*/
static void drainLoop(void)
{
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
/*----------------------------------------------------------------------------*/
static bool isTestFinished(void *argument)
{
  struct Context * const context = argument;
  return context->count >= MAX_ITERATIONS;
}
/*----------------------------------------------------------------------------*/
static void periodicTask(void *argument)
{
  struct Context * const context = argument;

  if (!context->limit || context->count < context->limit)
  {
    ck_assert_uint_lt(context->count, ARRAY_SIZE(context->timestamps));
    context->timestamps[context->count] = timerGetValue(context->factory);

    if (context->autostop)
    {
      /*
       * The autostop timer is restarted with the next test period,
       * the delta of the next firing must match it.
       */
      context->period =
          testTaskPeriods[(context->count + 1) % ARRAY_SIZE(testTaskPeriods)];

      timerSetOverflow(context->timer, context->period);
      timerEnable(context->timer);
    }

    ++context->count;
  }
}
/*----------------------------------------------------------------------------*/
static bool waitForCompletion(bool (*condition)(void *), void *argument,
    uint64_t ms)
{
  uv_loop_t * const loop = uv_default_loop();
  const uint64_t deadline = uv_now(loop) + ms;

  do
  {
    uv_run(loop, UV_RUN_NOWAIT);

    if (condition(argument))
      return true;

    udelay(1000);
  }
  while (uv_now(loop) < deadline);

  uv_run(loop, UV_RUN_NOWAIT);

  return condition(argument);
}
/*----------------------------------------------------------------------------*/
START_TEST(testFactoryRestarts)
{
  struct Context context[MAX_TIMERS] = {};
  struct Timer *timers[MAX_TIMERS] = {};

  struct Timer * const base = init(Timer, nullptr);
  ck_assert_ptr_nonnull(base);

  const struct TimerFactoryConfig factoryConfig = {
      .timer = base
  };
  struct TimerFactory * const factory = init(TimerFactory, &factoryConfig);
  ck_assert_ptr_nonnull(factory);
  timerSetOverflow(factory, timerGetFrequency(factory) / 1000);

  for (size_t i = 0; i < ARRAY_SIZE(context); ++i)
  {
    timers[i] = timerFactoryCreate(factory);
    ck_assert_ptr_nonnull(timers[i]);

    context[i].factory = (struct Timer *)factory;
    context[i].timer = timers[i];
    context[i].limit = (i == 0) ? MAX_ITERATIONS : 0;
    context[i].period = TICKS_PER_QUANTA + 10 * i * i;
    context[i].autostop = (i == 0);

    timerSetAutostop(timers[i], context[i].autostop);
    timerSetCallback(timers[i], periodicTask, context + i);
    timerSetOverflow(timers[i], context[i].period);
    timerEnable(timers[i]);
  }
  timerEnable(factory);

  /* The restarted timer terminates the test run */
  const bool finished = waitForCompletion(isTestFinished, &context[0],
      WAIT_TIMEOUT_MS);

  /* Factory entries must be removed before the factory deinit */
  for (size_t i = 0; i < ARRAY_SIZE(context); ++i)
    deinit(timers[i]);
  deinit(factory);
  deinit(base);
  drainLoop();

  ck_assert(finished == true);
  ck_assert_uint_eq(context[0].count, MAX_ITERATIONS);
  checkVariablePeriods(&context[0]);

  /* Continuous timers keep their fixed period while the first restarts */
  for (size_t i = 1; i < ARRAY_SIZE(context); ++i)
    checkPeriods(&context[i]);
}
END_TEST
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  Suite * const suite = suite_create("TimerFactoryRestarts");
  TCase * const testcase = tcase_create("Core");

  tcase_add_test(testcase, testFactoryRestarts);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
