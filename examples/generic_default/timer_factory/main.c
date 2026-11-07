/*
 * generic_default/timer_factory/main.c
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
#define MAX_ITERATIONS   100
#define MAX_TIMERS       4
#define TICKS_PER_QUANTA 11
#define WAIT_TIMEOUT_MS  5000
/*----------------------------------------------------------------------------*/
struct Context
{
  struct Timer *factory;
  uint32_t count;
  uint32_t period;
  uint32_t timestamps[WAIT_TIMEOUT_MS / TICKS_PER_QUANTA];
};
/*----------------------------------------------------------------------------*/
static void checkPeriods(struct Context *);
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

  context->timestamps[context->count] = timerGetValue(context->factory);
  ++context->count;
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
START_TEST(testFactoryTimers)
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
  timerSetFrequency(factory, 1000);
  timerSetOverflow(factory, timerGetFrequency(factory) / 1000);
  ck_assert_uint_eq(timerGetFrequency(factory), 1000);
  ck_assert_uint_eq(timerGetOverflow(factory), 1);

  for (size_t i = 0; i < ARRAY_SIZE(context); ++i)
  {
    timers[i] = timerFactoryCreate(factory);
    ck_assert_ptr_nonnull(timers[i]);

    context[i].factory = (struct Timer *)factory;
    context[i].period = TICKS_PER_QUANTA + 10 * i * i;

    timerSetCallback(timers[i], periodicTask, context + i);
    timerSetOverflow(timers[i], context[i].period);
    ck_assert_uint_eq(timerGetFrequency(timers[i]), 1000);
    ck_assert_uint_eq(timerGetOverflow(timers[i]), context[i].period);
    ck_assert_uint_eq(timerGetValue(timers[i]), 0);

    timerEnable(timers[i]);
  }

  /* The shortest period timer terminates the test run */
  timerEnable(factory);
  const bool finished = waitForCompletion(isTestFinished, &context[0],
      WAIT_TIMEOUT_MS);
  timerDisable(factory);

  /* Factory entries must be removed before the factory deinit */
  for (size_t i = 0; i < ARRAY_SIZE(context); ++i)
    deinit(timers[i]);
  deinit(factory);
  deinit(base);
  drainLoop();

  ck_assert(finished == true);
  ck_assert_uint_eq(context[0].count, MAX_ITERATIONS);
  for (size_t i = 0; i < ARRAY_SIZE(context); ++i)
    checkPeriods(&context[i]);

  /* Other timers fire proportionally to their periods */
  for (size_t i = 1; i < ARRAY_SIZE(context); ++i)
  {
    const uint32_t maxIterationsScaled =
        MAX_ITERATIONS * TICKS_PER_QUANTA / (TICKS_PER_QUANTA + 10 * i * i);

    ck_assert_uint_ge(context[i].count, maxIterationsScaled - 1);
    ck_assert_uint_le(context[i].count, maxIterationsScaled + 1);
  }
}
END_TEST
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  Suite * const suite = suite_create("TimerFactory");
  TCase * const testcase = tcase_create("Core");

  tcase_add_test(testcase, testFactoryTimers);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
