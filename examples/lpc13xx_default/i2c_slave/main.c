/*
 * lpc13xx_default/i2c_slave/main.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/i2c_slave.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(3, 0)
/*----------------------------------------------------------------------------*/
static const struct I2CSlaveConfig i2cConfig = {
    .size = 16,
    .scl = PIN(0, 4),
    .sda = PIN(0, 5),
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static void onDeviceMemoryChanged(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const uint32_t deviceAddress = 0x60;
  static const uint32_t internalOffset = 0;
  enum Result res;

  struct Interface * const i2c = init(I2CSlave, &i2cConfig);
  assert(i2c);
  ifSetParam(i2c, IF_ADDRESS, &deviceAddress);

  (void)res; /* Suppress warning */

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  bool event = false;
  ifSetCallback(i2c, onDeviceMemoryChanged, &event);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    uint8_t state;

    ifSetParam(i2c, IF_POSITION, &internalOffset);
    ifRead(i2c, &state, sizeof(state));
    pinWrite(led, state > 0);
  }

  return 0;
}
