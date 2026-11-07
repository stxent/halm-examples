/*
 * generic_default/task_sequence/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/delay.h>
#include <halm/generic/task_sequence.h>
#include <halm/platform/generic/event_queue.h>
#include <halm/platform/generic/timer.h>
#include <check.h>
#include <uv.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#define SEQUENCE_ITERATIONS  3
#define TASK_COUNT           3
#define DELAY_1_MS           20
#define DELAY_2_MS           40
#define WAIT_TIMEOUT_MS      5000
/*----------------------------------------------------------------------------*/
struct Context
{
  struct TaskSequence *sequence;
  struct Timer *chrono;
  uint32_t timestamps[SEQUENCE_ITERATIONS * TASK_COUNT];
  uint32_t count;
  uint32_t overflows;
  uint32_t started;
  uint32_t busy;
};
/*----------------------------------------------------------------------------*/
static void taskEmpty(void *);
static void taskRecord(void *);
static void onEventOverflow(void *);
static bool sequenceFinished(void *);
static bool sequenceCompleted(void *);
static bool waitForCompletion(bool (*)(void *), void *, uint64_t);
static void drainLoop(void);
/*----------------------------------------------------------------------------*/
static void taskEmpty(void *)
{
}
/*----------------------------------------------------------------------------*/
static void taskRecord(void *argument)
{
  struct Context * const context = argument;

  context->timestamps[context->count] = timerGetValue(context->chrono);
  ++context->count;
}
/*----------------------------------------------------------------------------*/
static void onEventOverflow(void *argument)
{
  struct Context * const context = argument;
  const enum Result res = tsStart(context->sequence);

  if (res == E_OK)
    ++context->started;
  else if (res == E_BUSY)
    ++context->busy;

  ++context->overflows;
}
/*----------------------------------------------------------------------------*/
static bool sequenceFinished(void *argument)
{
  struct Context * const context = argument;

  return context->overflows >= SEQUENCE_ITERATIONS
      && tsIsCompleted(context->sequence);
}
/*----------------------------------------------------------------------------*/
static bool sequenceCompleted(void *argument)
{
  return tsIsCompleted(argument);
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

    udelay(100);
  }
  while (uv_now(loop) < deadline);

  uv_run(loop, UV_RUN_NOWAIT);

  return condition(argument);
}
/*----------------------------------------------------------------------------*/
static void drainLoop(void)
{
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
/*----------------------------------------------------------------------------*/
START_TEST(testSequenceExecution)
{
  struct Context context = {};

  WQ_DEFAULT = init(EventQueue, nullptr);
  ck_assert_ptr_nonnull(WQ_DEFAULT);
  ck_assert_uint_eq(wqStart(WQ_DEFAULT), E_OK);

  struct Timer * const chrono = init(Timer, nullptr);
  ck_assert_ptr_nonnull(chrono);
  context.chrono = chrono;

  struct Timer * const taskTimer = init(Timer, nullptr);
  ck_assert_ptr_nonnull(taskTimer);

  const struct TaskSequenceConfig config = {
      .timer = taskTimer,
      .wq = WQ_DEFAULT,
      .size = 10
  };
  struct TaskSequence * const sequence = init(TaskSequence, &config);
  ck_assert_ptr_nonnull(sequence);
  context.sequence = sequence;

  ck_assert_uint_eq(tsAdd(sequence, taskRecord, &context, DELAY_1_MS), E_OK);
  ck_assert_uint_eq(tsAdd(sequence, taskRecord, &context, DELAY_2_MS), E_OK);
  ck_assert_uint_eq(tsAdd(sequence, taskRecord, &context, DELAY_1_MS), E_OK);

  struct Timer * const eventTimer = init(Timer, nullptr);
  ck_assert_ptr_nonnull(eventTimer);
  timerSetOverflow(eventTimer, timerGetFrequency(eventTimer) / 10);
  timerSetCallback(eventTimer, onEventOverflow, &context);
  timerEnable(chrono);
  timerEnable(eventTimer);

  const bool finished = waitForCompletion(sequenceFinished, &context,
      WAIT_TIMEOUT_MS);
  const bool completed = tsIsCompleted(sequence);

  deinit(eventTimer);
  deinit(sequence);
  deinit(chrono);
  deinit(taskTimer);
  drainLoop();

  ck_assert(finished == true);
  ck_assert(completed == true);
  ck_assert_uint_eq(context.started, SEQUENCE_ITERATIONS);
  ck_assert_uint_eq(context.busy, 0);
  ck_assert_uint_eq(context.count, SEQUENCE_ITERATIONS * TASK_COUNT);

  /* Tasks run in order with the configured delays between them */
  for (uint32_t cycle = 0; cycle < SEQUENCE_ITERATIONS; ++cycle)
  {
    const uint32_t first = context.timestamps[3 * cycle];
    const uint32_t second = context.timestamps[3 * cycle + 1];
    const uint32_t third = context.timestamps[3 * cycle + 2];

    ck_assert_uint_ge(second - first, DELAY_1_MS - 1);
    ck_assert_uint_le(second - first, DELAY_1_MS + 1);
    ck_assert_uint_ge(third - second, DELAY_2_MS - 1);
    ck_assert_uint_le(third - second, DELAY_2_MS + 1);
  }
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testTaskQueueEdges)
{
  WQ_DEFAULT = init(EventQueue, nullptr);
  ck_assert_ptr_nonnull(WQ_DEFAULT);
  ck_assert_uint_eq(wqStart(WQ_DEFAULT), E_OK);

  struct Timer * const timer = init(Timer, nullptr);
  ck_assert_ptr_nonnull(timer);

  const struct TaskSequenceConfig config = {
      .timer = timer,
      .wq = WQ_DEFAULT,
      .size = 2
  };
  struct TaskSequence * const sequence = init(TaskSequence, &config);
  ck_assert_ptr_nonnull(sequence);

  /* An empty sequence cannot be started */
  ck_assert_uint_eq(tsStart(sequence), E_EMPTY);

  ck_assert_uint_eq(tsAdd(sequence, taskEmpty, nullptr, 0), E_OK);
  ck_assert_uint_eq(tsAdd(sequence, taskEmpty, nullptr, 0), E_OK);
  ck_assert_uint_eq(tsAdd(sequence, taskEmpty, nullptr, 0), E_FULL);

  ck_assert_uint_eq(tsStart(sequence), E_OK);
  ck_assert_uint_eq(tsStart(sequence), E_BUSY);

  /* Zero delay tasks complete the sequence without timer events */
  const bool finished = waitForCompletion(sequenceCompleted, sequence,
      WAIT_TIMEOUT_MS);

  tsStop(sequence);
  tsClear(sequence);
  drainLoop();

  ck_assert(finished == true);
  ck_assert_uint_eq(tsTaskArraySize(&sequence->tasks), 0);

  deinit(sequence);
  deinit(timer);
}
END_TEST
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  Suite * const suite = suite_create("TaskSequence");
  TCase * const testcase = tcase_create("Core");

  tcase_add_test(testcase, testTaskQueueEdges);
  tcase_add_test(testcase, testSequenceExecution);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
