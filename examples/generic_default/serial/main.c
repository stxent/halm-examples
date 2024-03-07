/*
 * generic_default/serial/main.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/platform/generic/serial.h>
#include <halm/platform/generic/signal_handler.h>
#include <halm/platform/generic/timer.h>
#include <uv.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
struct SerialEventTuple
{
  struct Interface *serial;
  size_t counter;
};

#define BUFFER_SIZE     64
#define MESSAGE_SIZE    16
#define MAX_ITERATIONS  10
/*----------------------------------------------------------------------------*/
static inline uint8_t binToHex(uint8_t);
static void onSerialEvent(void *);
static void onSignalReceived(void *);
static void onTimerOverflow(void *);
static void onUvWalk(uv_handle_t *, void *);
/*----------------------------------------------------------------------------*/
static inline uint8_t binToHex(uint8_t value)
{
  return value + (value < 10 ? 0x30 : 0x37);
}
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  struct SerialEventTuple * const context = argument;
  char buffer[BUFFER_SIZE];

  const size_t length = ifRead(context->serial, buffer, sizeof(buffer));
  context->counter += length;

  if (length > 0)
  {
    char serialized[BUFFER_SIZE * 4];
    strcpy(serialized, "RX: [");

    char *position = serialized + strlen(serialized);

    for (size_t i = 0; i < length; ++i)
    {
      *position++ = binToHex(buffer[i] >> 4);
      *position++ = binToHex(buffer[i] & 0x0F);

      if (i != length - 1)
        *position++ = ' ';
    }
    strcpy(position, "]\r\n");

    printf(serialized);
  }
}
/*----------------------------------------------------------------------------*/
static void onSignalReceived(void *argument)
{
  uv_walk(argument, onUvWalk, NULL);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  static unsigned long iteration = 0;

  if (iteration < MAX_ITERATIONS)
  {
    char buffer[BUFFER_SIZE];

    sprintf(buffer, "Test %09lu\r\n", iteration);
    ++iteration;

    printf("TX: %s", buffer);
    ifWrite(argument, buffer, strlen(buffer));
  }
  else
    raise(SIGINT);
}
/*----------------------------------------------------------------------------*/
static void onUvWalk(uv_handle_t *handle, [[maybe_unused]] void *argument)
{
  deinit(uv_handle_get_data(handle));
}
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  if (argc != 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
  {
    printf("Usage: serial PORT\r\n");
    printf("  -h, --help  print help message\r\n");
    exit(EXIT_FAILURE);
  }

  uv_loop_t * const loop = uv_default_loop();

  /* Serial port */
  struct SerialEventTuple context = {
      .serial = 0,
      .counter = 0
  };
  const struct SerialConfig serialConfig = {
      .device = argv[1],
      .rate = 19200,
      .parity = SERIAL_PARITY_NONE
  };
  struct Interface * const serial = init(Serial, &serialConfig);

  if (serial != NULL)
  {
    context.serial = serial;
    ifSetCallback(serial, onSerialEvent, &context);

    /* SIGINT listener */
    const struct SignalHandlerConfig listenerConfig = {
        .signum = SIGINT
    };
    struct SignalHandler * const listener = init(SignalHandler,
        &listenerConfig);
    assert(listener != NULL);
    interruptSetCallback(listener, onSignalReceived, loop);
    interruptEnable(listener);

    /* Periodic timer */
    struct Timer * const timer = init(Timer, NULL);
    assert(timer != NULL);
    timerSetOverflow(timer, 100);
    timerSetCallback(timer, onTimerOverflow, serial);
    timerEnable(timer);
  }
  else
    printf("Open failed: %s\r\n", argv[1]);

  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);

  if (!serial || context.counter != MAX_ITERATIONS * MESSAGE_SIZE)
  {
    printf("Received %zu, expected %zu\r\n", context.counter,
        (size_t)(MAX_ITERATIONS * MESSAGE_SIZE));
    return EXIT_FAILURE;
  }
  else
    return EXIT_SUCCESS;
}
