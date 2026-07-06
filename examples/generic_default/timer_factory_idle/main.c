/*
 * generic_default/timer_factory_idle/main.c
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
#define MAX_TIMERS     3

struct Context
{
  struct Timer *chrono;
  struct Timer *timer;
  unsigned int iteration;
  unsigned int limit;
  unsigned long period;
  char key;
};
/*----------------------------------------------------------------------------*/
static void onBurstRequest(void *);
static void onSignalReceived(void *);
static void onUvWalk(uv_handle_t *, void *);
static void periodicTask(void *);
/*----------------------------------------------------------------------------*/
static void onBurstRequest(void *argument)
{
  static size_t iteration = 0;
  struct Context * const contexts = argument;

  /* Enable timers in a pseudo-random order */
  for (size_t i = 0; i < MAX_TIMERS; ++i)
    timerEnable(contexts[(iteration + i) % MAX_TIMERS].timer);
  ++iteration;
}
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
    const unsigned long timestamp = timerGetValue(context->chrono);

    printf("%09lu Task %c %3u\r\n", timestamp, context->key,
        context->iteration);

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

  /* Periodic timer */
  struct Timer * const periodicTimer = init(Timer, NULL);
  assert(periodicTimer != NULL);
  timerSetOverflow(periodicTimer, 100);
  timerSetCallback(periodicTimer, onBurstRequest, context);

  /* Timer factory */
  const struct TimerFactoryConfig factoryConfig = {
      .timer = init(Timer, NULL)
  };
  assert(factoryConfig.timer != NULL);
  struct TimerFactory * const timerFactory = init(TimerFactory, &factoryConfig);
  assert(timerFactory != NULL);
  timerSetOverflow(timerFactory, timerGetFrequency(timerFactory) / 1000);

  for (size_t i = 0; i < ARRAY_SIZE(context); ++i)
  {
    context[i].period = 9 + 10 * i * i;
    context[i].timer = timerFactoryCreate(timerFactory);
    assert(context[i].timer != NULL);
    timerSetAutostop(context[i].timer, true);
    timerSetCallback(context[i].timer, periodicTask, context + i);
    timerSetOverflow(context[i].timer, context[i].period);

    context[i].chrono = (struct Timer *)timerFactory;
    context[i].iteration = 0;
    context[i].limit = 0;
    context[i].key = 'A' + i;
  }
  context[0].limit = MAX_ITERATIONS;

  timerEnable(timerFactory);
  timerEnable(periodicTimer);
  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  return EXIT_SUCCESS;
}
