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
  uv_walk(argument, onUvWalk, NULL);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *)
{
  const enum Result res = wqAdd(WQ_DEFAULT, periodicTask, NULL);

  assert(res == E_OK);
  (void)res;
}
/*----------------------------------------------------------------------------*/
static void onUvWalk(uv_handle_t *handle, void *)
{
  deinit(uv_handle_get_data(handle));
}
/*----------------------------------------------------------------------------*/
static void periodicTask(void *)
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
int main(int, char *[])
{
  uv_loop_t * const loop = uv_default_loop();

  /* SIGINT listener */
  const struct SignalHandlerConfig listenerConfig = {
      .signum = SIGINT
  };
  struct SignalHandler * const listener = init(SignalHandler, &listenerConfig);
  assert(listener != NULL);
  interruptSetCallback(listener, onSignalReceived, loop);
  interruptEnable(listener);

  /* Periodic timer */
  struct Timer * const timer = init(Timer, NULL);
  assert(timer != NULL);
  timerSetOverflow(timer, 100);
  timerSetCallback(timer, onTimerOverflow, NULL);
  timerEnable(timer);

  /* Initialize Work Queue */
  WQ_DEFAULT = init(EventQueue, NULL);
  assert(WQ_DEFAULT);
  wqStart(WQ_DEFAULT);

  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  return EXIT_SUCCESS;
}
