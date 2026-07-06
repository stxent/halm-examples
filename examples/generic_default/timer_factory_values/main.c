/*
 * generic_default/timer_factory_values/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/timer_factory.h>
#include <halm/platform/generic/signal_handler.h>
#include <halm/platform/generic/timer.h>
#include <uv.h>
#include <assert.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#define MAX_ITERATIONS 10
#define PERIOD_A       100
#define PERIOD_B       40
#define PERIOD_C       79

struct Context
{
  struct Timer *chrono;
  struct Timer *target;
};
/*----------------------------------------------------------------------------*/
static void onSignalReceived(void *);
static void onUvWalk(uv_handle_t *, void *);
static void periodicTaskA(void *);
static void periodicTaskB(void *);
static void periodicTaskC(void *);
/*----------------------------------------------------------------------------*/
static void onSignalReceived(void *argument)
{
  uv_walk(argument, onUvWalk, NULL);
}
/*----------------------------------------------------------------------------*/
static void onUvWalk(uv_handle_t *handle, void *)
{
  deinit(uv_handle_get_data(handle));
}
/*----------------------------------------------------------------------------*/
static void periodicTaskA(void *argument)
{
  static unsigned int iteration = 0;

  if (iteration < MAX_ITERATIONS)
  {
    struct Context * const context = argument;
    const unsigned long timestamp = timerGetValue(context->chrono);
    const unsigned long value = timerGetValue(context->target);
    timerSetValue(context->target, 0);

    printf("%09lu Task B reset at %lu\r\n", timestamp, value);
    ++iteration;
  }
  else
    raise(SIGINT);
}
/*----------------------------------------------------------------------------*/
static void periodicTaskB(void *argument)
{
  static unsigned int failures = 0;
  const unsigned long timestamp = timerGetValue(argument);
  const unsigned long offset = timestamp % 100;
  const bool failure = offset != 40 && offset != 80;

  printf("%09lu Task B offset %lu%s\r\n", timestamp, offset,
      failure ? " UNSYNC" : "");
  if (failure)
    ++failures;
}
/*----------------------------------------------------------------------------*/
static void periodicTaskC(void *argument)
{
  const unsigned long timestamp = timerGetValue(argument);
  printf("%09lu Task C\r\n", timestamp);
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

  /* Chrono timer */
  struct Timer * const chrono = init(Timer, NULL);
  assert(chrono != NULL);
  timerEnable(chrono);

  /* Timer factory */
  const struct TimerFactoryConfig factoryConfig = {
      .timer = init(Timer, NULL)
  };
  assert(factoryConfig.timer != NULL);
  struct TimerFactory * const timerFactory = init(TimerFactory, &factoryConfig);
  assert(timerFactory != NULL);
  timerSetOverflow(timerFactory, timerGetFrequency(timerFactory) / 1000);

  struct Timer * const timerB = timerFactoryCreate(timerFactory);
  assert(timerB != NULL);
  timerSetCallback(timerB, periodicTaskB, timerFactory);
  timerSetOverflow(timerB, PERIOD_B);
  timerEnable(timerB);

  struct Timer * const timerA = timerFactoryCreate(timerFactory);
  assert(timerA != NULL);

  struct Context context = {(struct Timer *)timerFactory, timerB};
  timerSetCallback(timerA, periodicTaskA, &context);
  timerSetOverflow(timerA, PERIOD_A);
  timerEnable(timerA);

  struct Timer * const timerC = timerFactoryCreate(timerFactory);
  assert(timerC != NULL);
  timerSetCallback(timerC, periodicTaskC, timerFactory);
  timerSetOverflow(timerC, PERIOD_C);
  timerEnable(timerC);

  timerEnable(timerFactory);
  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  return EXIT_SUCCESS;
}
