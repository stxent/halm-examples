/*
 * lpc17xx_default/i2c/main.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/timer.h>
#include <xcore/interface.h>
#include <xcore/memory.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define ADDRESS_SIZE    1
#define DEVICE_ADDRESS  0x60
#define DEVICE_RATE     400000
/*----------------------------------------------------------------------------*/
static bool checkBuffer(uint8_t *buffer, size_t length)
{
  for (size_t i = 0; i < length; ++i)
  {
    if (buffer[i] != i)
      return false;
  }

  return true;
}
/*----------------------------------------------------------------------------*/
static void fillBuffer(uint8_t *buffer, size_t length)
{
  for (size_t i = 0; i < length; ++i)
    buffer[i] = i;
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
  deviceConfigIO(interface);

  if (ifWrite(interface, &position, ADDRESS_SIZE) == ADDRESS_SIZE)
    ifRead(interface, buffer, length);
}
/*----------------------------------------------------------------------------*/
static void deviceWrite(struct Interface *interface, uint32_t position,
    const void *buffer, size_t length)
{
  uint8_t packet[length + ADDRESS_SIZE];

  memcpy(packet, &position, ADDRESS_SIZE);
  memcpy(packet + ADDRESS_SIZE, buffer, length);

  deviceConfigIO(interface);

  ifWrite(interface, packet, length + ADDRESS_SIZE);
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
  bool event = false;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, true);

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

    fillBuffer(buffer, sizeof(buffer));
    deviceWrite(i2c, 0, buffer, sizeof(buffer));

    while (!event)
      barrier();
    event = false;

    memset(buffer, 0, sizeof(buffer));
    deviceRead(i2c, 0, buffer, sizeof(buffer));
    if (checkBuffer(buffer, sizeof(buffer)))
      pinReset(led);
  }

  return 0;
}
