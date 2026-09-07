/*
 * generic_default/task_sequence/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/task_sequence.h>
#include <halm/platform/generic/event_queue.h>
#include <halm/platform/generic/signal_handler.h>
#include <halm/platform/generic/timer.h>
#include <uv.h>
#include <assert.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#define MAX_ITERATIONS 3
/*----------------------------------------------------------------------------*/
static void taskPrintExit(void *);
static void taskPrintHello(void *);
static void taskPrintNewline(void *);
static void taskPrintWorld(void *);
static void onSignalReceived(void *);
static void onTimerOverflow(void *);
static void onUvWalk(uv_handle_t *, void *);
/*----------------------------------------------------------------------------*/
static void taskPrintExit(void *)
{
  printf("Exit\r\n");
  raise(SIGINT);
}
/*----------------------------------------------------------------------------*/
static void taskPrintHello(void *)
{
  printf("Hello ");
  fflush(stdout);
}
/*----------------------------------------------------------------------------*/
static void taskPrintNewline(void *)
{
  printf("\r\n");
}
/*----------------------------------------------------------------------------*/
static void taskPrintWorld(void *)
{
  printf("world");
  fflush(stdout);
}
/*----------------------------------------------------------------------------*/
static void onSignalReceived(void *argument)
{
  uv_walk(argument, onUvWalk, nullptr);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  static unsigned long iteration = 0;

  if (iteration == MAX_ITERATIONS)
    tsAdd(argument, taskPrintExit, nullptr, 0);

  [[maybe_unused]] const enum Result res = tsStart(argument);
  assert(res == E_OK);

  ++iteration;
}
/*----------------------------------------------------------------------------*/
static void onUvWalk(uv_handle_t *handle, void *)
{
  deinit(uv_handle_get_data(handle));
}
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  uv_loop_t * const loop = uv_default_loop();

  /* SIGINT listener */
  const struct SignalHandlerConfig listenerConfig = {
      .signum = SIGINT
  };
  struct SignalHandler * const listener = init(SignalHandler, &listenerConfig);
  assert(listener != nullptr);
  interruptSetCallback(listener, onSignalReceived, loop);
  interruptEnable(listener);

  /* Initialize Work Queue */
  WQ_DEFAULT = init(EventQueue, nullptr);
  assert(WQ_DEFAULT);

  /* Task timer */
  struct Timer * const taskTimer = init(Timer, nullptr);
  assert(taskTimer != nullptr);

  /* Task sequence */
  const struct TaskSequenceConfig sequenceConfig = {
      .timer = taskTimer,
      .wq = WQ_DEFAULT,
      .size = 10
  };
  struct TaskSequence * const sequence = init(TaskSequence, &sequenceConfig);
  assert(sequence != nullptr);

  tsAdd(sequence, taskPrintHello, nullptr, 200);
  tsAdd(sequence, taskPrintWorld, nullptr, 400);
  tsAdd(sequence, taskPrintNewline, nullptr, 200);

  /* Periodic timer */
  struct Timer * const eventTimer = init(Timer, nullptr);
  assert(eventTimer != nullptr);
  timerSetOverflow(eventTimer, timerGetFrequency(eventTimer));
  timerSetCallback(eventTimer, onTimerOverflow, sequence);
  timerEnable(eventTimer);

  wqStart(WQ_DEFAULT);
  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  return EXIT_SUCCESS;
}
