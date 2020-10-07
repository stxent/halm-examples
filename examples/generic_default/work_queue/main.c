/*
 * generic_default/work_queue/main.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/platform/generic/event_queue.h>
#include <halm/platform/generic/signal_handler.h>
#include <halm/platform/generic/timer.h>
#include <uv.h>
#include <assert.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#define MAX_ITERATIONS 10
/*----------------------------------------------------------------------------*/
static void onSignalReceived(void *);
static void onTimerOverflow(void *);
static void onUvWalk(uv_handle_t *, void *);
static void periodicTask(void *);
/*----------------------------------------------------------------------------*/
static void onSignalReceived(void *argument)
{
  uv_walk(argument, onUvWalk, 0);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument __attribute__((unused)))
{
  const enum Result res = wqAdd(WQ_DEFAULT, periodicTask, 0);

  assert(res == E_OK);
  (void)res;
}
/*----------------------------------------------------------------------------*/
static void onUvWalk(uv_handle_t *handle,
    void *argument __attribute__((unused)))
{
  deinit(uv_handle_get_data(handle));
}
/*----------------------------------------------------------------------------*/
static void periodicTask(void *argument __attribute__((unused)))
{
  static unsigned long iteration = 0;

  if (iteration < MAX_ITERATIONS)
  {
    printf("Test %09lu\r\n", iteration);
    ++iteration;
  }
  else
    raise(SIGINT);
}
/*----------------------------------------------------------------------------*/
int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
  uv_loop_t * const loop = uv_default_loop();

  /* SIGINT listener */
  const struct SignalHandlerConfig listenerConfig = {
      .signum = SIGINT
  };
  struct SignalHandler * const listener = init(SignalHandler, &listenerConfig);
  assert(listener);
  interruptSetCallback(listener, onSignalReceived, loop);
  interruptEnable(listener);

  /* Periodic timer */
  struct Timer * const timer = init(Timer, 0);
  assert(timer);
  timerSetOverflow(timer, 100);
  timerSetCallback(timer, onTimerOverflow, 0);
  timerEnable(timer);

  /* Initialize Work Queue */
  WQ_DEFAULT = init(EventQueue, 0);
  assert(WQ_DEFAULT);
  wqStart(WQ_DEFAULT);

  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  return EXIT_SUCCESS;
}
