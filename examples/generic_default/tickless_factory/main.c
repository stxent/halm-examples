/*
 * generic_default/tickless_factory/main.c
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
#define MAX_ITERATIONS 2
#define MAX_TIMERS     4

struct Context
{
  struct Timer *chrono;
  struct Timer *timer;
  unsigned int failures;
  unsigned int iteration;
  unsigned int limit;
  unsigned long period;
  unsigned long previous;
  char key;
};
/*----------------------------------------------------------------------------*/
static void onSignalReceived(void *);
static void onUvWalk(uv_handle_t *, void *);
static void periodicTask(void *);
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
static void periodicTask(void *argument)
{
  struct Context * const context = argument;

  if (!context->limit || context->iteration < context->limit)
  {
    const unsigned long overflow = timerGetOverflow(context->chrono);
    const unsigned long timestamp = timerGetValue(context->chrono);
    long delta = (long)(timestamp - context->previous);

    if (delta < 0)
      delta += overflow;

    const bool failure = delta != (long)context->period;

    printf("%09lu Task %c %3u delta %lu%s\r\n", timestamp, context->key,
        context->iteration, delta, failure ? " UNSYNC" : "");

    context->previous = timestamp;
    if (failure)
      ++context->failures;
    ++context->iteration;
  }
  else
    raise(SIGINT);
}
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  struct Context context[MAX_TIMERS];
  uv_loop_t * const loop = uv_default_loop();

  /* SIGINT listener */
  const struct SignalHandlerConfig listenerConfig = {
      .signum = SIGINT
  };
  struct SignalHandler * const listener = init(SignalHandler, &listenerConfig);
  assert(listener != NULL);
  interruptSetCallback(listener, onSignalReceived, loop);
  interruptEnable(listener);

  /* Timer factory */
  const struct TimerConfig timerConfig = {
      .frequency = 0,
      .resolution = 1000,
      .freerun = true
  };
  const struct TimerFactoryConfig factoryConfig = {
      .timer = init(Timer, &timerConfig)
  };
  assert(factoryConfig.timer != NULL);
  struct TimerFactory * const timerFactory =
      init(TicklessFactory, &factoryConfig);
  assert(timerFactory != NULL);

  for (size_t i = 0; i < ARRAY_SIZE(context); ++i)
  {
    context[i].period = (i == ARRAY_SIZE(context) - 1) ? 500 : 7 + 10 * i * i;

    context[i].timer = timerFactoryCreate(timerFactory);
    assert(context[i].timer != NULL);
    timerSetCallback(context[i].timer, periodicTask, context + i);
    timerSetOverflow(context[i].timer, context[i].period);
    timerEnable(context[i].timer);

    context[i].chrono = (struct Timer *)timerFactory;
    context[i].failures = 0;
    context[i].iteration = 0;
    context[i].limit = 0;
    context[i].previous = 0;
    context[i].key = 'A' + i;
  }
  context[ARRAY_SIZE(context) - 1].limit = MAX_ITERATIONS;

  printf("Factory overflow  %lu\r\n",
      (unsigned long)timerGetOverflow(timerFactory));
  printf("Factory frequency %lu\r\n",
      (unsigned long)timerGetFrequency(timerFactory));
  printf("Timer frequency   %lu\r\n",
      (unsigned long)timerGetFrequency(context[0].timer));

  timerEnable(timerFactory);
  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  return EXIT_SUCCESS;
}
