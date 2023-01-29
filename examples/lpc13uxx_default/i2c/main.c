/*
 * lpc13uxx_default/i2c/main.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/timer.h>
#include <xcore/interface.h>
#include <xcore/memory.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define ADDRESS_SIZE    2
#define DEVICE_ADDRESS  0x50
#define DEVICE_RATE     400000
/*----------------------------------------------------------------------------*/
static bool checkBuffer(uint8_t *buffer, size_t length, uint8_t iteration)
{
  for (size_t i = 0; i < length; ++i)
  {
    if (buffer[i] != (uint8_t)(iteration + i))
      return false;
  }

  return true;
}
/*----------------------------------------------------------------------------*/
static void fillBuffer(uint8_t *buffer, size_t length, uint8_t iteration)
{
  for (size_t i = 0; i < length; ++i)
    buffer[i] = (uint8_t)(iteration + i);
}
/*----------------------------------------------------------------------------*/
static void deviceConfigIO(struct Interface *interface)
{
  const uint32_t address = DEVICE_ADDRESS;
  const uint32_t rate = DEVICE_RATE;
  enum Result res;

  res = ifSetParam(interface, IF_ADDRESS, &address);
  assert(res == E_OK);
  res = ifSetParam(interface, IF_RATE, &rate);
  assert(res == E_OK);

  (void)res; /* Suppress warning */
}
/*----------------------------------------------------------------------------*/
static void deviceRead(struct Interface *interface, uint32_t position,
    void *buffer, size_t length)
{
  uint8_t packet[ADDRESS_SIZE];

  for (size_t i = 0; i < ADDRESS_SIZE; ++i)
    packet[i] = position >> ((ADDRESS_SIZE - 1 - i) * 8);

  deviceConfigIO(interface);
  if (ifWrite(interface, packet, sizeof(packet)) == sizeof(packet))
    ifRead(interface, buffer, length);
}
/*----------------------------------------------------------------------------*/
static void deviceWrite(struct Interface *interface, uint32_t position,
    const void *buffer, size_t length)
{
  uint8_t packet[length + ADDRESS_SIZE];

  for (size_t i = 0; i < ADDRESS_SIZE; ++i)
    packet[i] = position >> ((ADDRESS_SIZE - 1 - i) * 8);
  memcpy(packet + ADDRESS_SIZE, buffer, length);

  deviceConfigIO(interface);
  ifWrite(interface, packet, sizeof(packet));
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  uint8_t buffer[16];
  uint8_t iteration = 0;
  bool event = false;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  struct Interface * const i2c = boardSetupI2C();

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer));
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);

    /* Write memory page */
    fillBuffer(buffer, sizeof(buffer), iteration);
    deviceWrite(i2c, 0, buffer, sizeof(buffer));

    /* Wait for page program operation */
    mdelay(10);

    /* Read and verify memory page */
    memset(buffer, 0, sizeof(buffer));
    deviceRead(i2c, 0, buffer, sizeof(buffer));

    if (checkBuffer(buffer, sizeof(buffer), iteration))
      pinReset(led);

    iteration += sizeof(buffer);
  }

  return 0;
}
