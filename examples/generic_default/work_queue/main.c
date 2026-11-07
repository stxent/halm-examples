/*
 * generic_default/work_queue/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/delay.h>
#include <halm/platform/generic/event_queue.h>
#include <halm/platform/generic/timer.h>
#include <check.h>
#include <uv.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#define MAX_ITERATIONS  10
#define WAIT_TIMEOUT_MS 5000
/*----------------------------------------------------------------------------*/
struct Context
{
  unsigned int executions;
  unsigned int failures;
};
/*----------------------------------------------------------------------------*/
static void drainLoop(void);
static bool isTestFinished(void *);
static void onTimerOverflow(void *);
static void periodicTask(void *);
static bool waitForCompletion(bool (*)(void *), void *, uint64_t);
/*----------------------------------------------------------------------------*/
static void drainLoop(void)
{
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
/*----------------------------------------------------------------------------*/
static bool isTestFinished(void *argument)
{
  struct Context * const context = argument;
  return context->executions >= MAX_ITERATIONS;
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  struct Context * const context = argument;

  if (context->executions < MAX_ITERATIONS
      && wqAdd(WQ_DEFAULT, periodicTask, context) != E_OK)
  {
    ++context->failures;
  }
}
/*----------------------------------------------------------------------------*/
static void periodicTask(void *argument)
{
  struct Context * const context = argument;
  ++context->executions;
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
START_TEST(testDirectAdd)
{
  WQ_DEFAULT = init(EventQueue, nullptr);
  ck_assert_ptr_nonnull(WQ_DEFAULT);

  struct Context context = {0};
  ck_assert_uint_eq(wqAdd(WQ_DEFAULT, periodicTask, &context), E_OK);
  drainLoop();

  ck_assert_uint_eq(context.executions, 1);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testPeriodicWorkQueue)
{
  WQ_DEFAULT = init(EventQueue, nullptr);
  ck_assert_ptr_nonnull(WQ_DEFAULT);
  ck_assert_uint_eq(wqStart(WQ_DEFAULT), E_OK);

  struct Context context = {0};
  struct Timer * const timer = init(Timer, nullptr);
  ck_assert_ptr_nonnull(timer);
  timerSetOverflow(timer, timerGetFrequency(timer) / 10);
  timerSetCallback(timer, onTimerOverflow, &context);
  timerEnable(timer);

  const bool finished = waitForCompletion(isTestFinished, &context,
      WAIT_TIMEOUT_MS);

  timerDisable(timer);
  deinit(timer);
  drainLoop();

  ck_assert(finished == true);
  ck_assert_uint_eq(context.failures, 0);
  ck_assert_uint_eq(context.executions, MAX_ITERATIONS);
}
END_TEST
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  Suite * const suite = suite_create("WorkQueue");
  TCase * const testcase = tcase_create("Core");

  tcase_add_test(testcase, testDirectAdd);
  tcase_add_test(testcase, testPeriodicWorkQueue);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
