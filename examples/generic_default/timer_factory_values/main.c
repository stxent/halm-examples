/*
 * generic_default/timer_factory_values/main.c
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
#define PERIOD_A         100
#define PERIOD_B         40
#define PERIOD_C         79
#define WAIT_TIMEOUT_MS  5000
/*----------------------------------------------------------------------------*/
struct Context
{
  struct Timer *factory;
  struct Timer *target;
  uint32_t count;
  uint32_t limit;
  uint32_t timestamps[WAIT_TIMEOUT_MS / PERIOD_B];
  uint32_t values[WAIT_TIMEOUT_MS / PERIOD_B];
};
/*----------------------------------------------------------------------------*/
static void checkPeriods(struct Context *, uint32_t);
static void drainLoop(void);
static bool isTestFinished(void *);
static void periodicTaskA(void *);
static void periodicTaskB(void *);
static void periodicTaskC(void *);
static bool waitForCompletion(bool (*)(void *), void *, uint64_t);
/*----------------------------------------------------------------------------*/
static void checkPeriods(struct Context *context, uint32_t period)
{
  for (uint32_t i = 1; i < context->count; ++i)
  {
    ck_assert_uint_eq(context->timestamps[i] - context->timestamps[i - 1],
        period);
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
static void periodicTaskA(void *argument)
{
  struct Context * const context = argument;

  if (context->count < context->limit)
  {
    context->timestamps[context->count] = timerGetValue(context->factory);
    context->values[context->count] = timerGetValue(context->target);

    /* Reset the target timer from inside the factory callback */
    timerSetValue(context->target, 0);
    ++context->count;
  }
}
/*----------------------------------------------------------------------------*/
static void periodicTaskB(void *argument)
{
  struct Context * const context = argument;

  context->timestamps[context->count] =
      timerGetValue(context->factory) % PERIOD_A;
  ++context->count;
}
/*----------------------------------------------------------------------------*/
static void periodicTaskC(void *argument)
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
START_TEST(testFactoryValues)
{
  struct Context contextA = {};
  struct Context contextB = {};
  struct Context contextC = {};

  struct Timer * const base = init(Timer, nullptr);
  ck_assert_ptr_nonnull(base);

  const struct TimerFactoryConfig factoryConfig = {
      .timer = base
  };
  struct TimerFactory * const factory = init(TimerFactory, &factoryConfig);
  ck_assert_ptr_nonnull(factory);
  timerSetOverflow(factory, timerGetFrequency(factory) / 1000);

  struct Timer * const timerB = timerFactoryCreate(factory);
  ck_assert_ptr_nonnull(timerB);
  contextB.factory = (struct Timer *)factory;
  timerSetCallback(timerB, periodicTaskB, &contextB);
  timerSetOverflow(timerB, PERIOD_B);
  timerEnable(timerB);

  struct Timer * const timerA = timerFactoryCreate(factory);
  ck_assert_ptr_nonnull(timerA);
  contextA.factory = (struct Timer *)factory;
  contextA.target = timerB;
  contextA.limit = MAX_ITERATIONS;
  timerSetCallback(timerA, periodicTaskA, &contextA);
  timerSetOverflow(timerA, PERIOD_A);
  timerEnable(timerA);

  struct Timer * const timerC = timerFactoryCreate(factory);
  ck_assert_ptr_nonnull(timerC);
  contextC.factory = (struct Timer *)factory;
  timerSetCallback(timerC, periodicTaskC, &contextC);
  timerSetOverflow(timerC, PERIOD_C);
  timerEnable(timerC);

  timerEnable(factory);

  const bool finished = waitForCompletion(isTestFinished, &contextA,
      WAIT_TIMEOUT_MS);

  /* Factory entries must be removed before the factory deinit */
  deinit(timerA);
  deinit(timerB);
  deinit(timerC);
  deinit(factory);
  deinit(base);
  drainLoop();

  ck_assert(finished == true);

  /* Timer A fires exactly on the 100 tick grid */
  ck_assert_uint_eq(contextA.count, MAX_ITERATIONS);
  checkPeriods(&contextA, PERIOD_A);
  for (uint32_t i = 0; i < contextA.count; ++i)
    ck_assert_uint_eq(contextA.values[i], PERIOD_A / 5);

  /* Timer C keeps its fixed period */
  ck_assert_uint_ge(contextC.count, PERIOD_A * MAX_ITERATIONS / PERIOD_C - 1);
  ck_assert_uint_le(contextC.count, PERIOD_A * MAX_ITERATIONS / PERIOD_C + 1);
  checkPeriods(&contextC, PERIOD_C);

  /*
   * Timer B overflows twice per A period and is reset 20 ticks after
   * its previous overflow on each A firing.
   */
  ck_assert_uint_ge(contextB.count, MAX_ITERATIONS * 2 - 1);
  ck_assert_uint_le(contextB.count, MAX_ITERATIONS * 2 + 1);
  for (uint32_t i = 0; i < contextB.count; ++i)
  {
    ck_assert(contextB.timestamps[i] == PERIOD_B
        || contextB.timestamps[i] == PERIOD_B * 2);
  }
}
END_TEST
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  Suite * const suite = suite_create("TimerFactoryValues");
  TCase * const testcase = tcase_create("Core");

  tcase_add_test(testcase, testFactoryValues);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
