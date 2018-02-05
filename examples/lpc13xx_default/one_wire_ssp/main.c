/*
 * lpc13xx_default/one_wire_ssp/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/one_wire_ssp.h>
#include <halm/platform/nxp/serial.h>
/*----------------------------------------------------------------------------*/
static const struct OneWireSspConfig owConfig = {
    .miso = PIN(0, 8),
    .mosi = PIN(0, 9),
    .channel = 0
};

static const struct SerialConfig serialConfig = {
    .rate = 19200,
    .rxLength = 64,
    .txLength = 64,
    .rx = PIN(1, 6),
    .tx = PIN(1, 7),
    .channel = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT32B0
};
/*----------------------------------------------------------------------------*/
static uint8_t binToHex(uint8_t value)
{
  return value < 10 ? value + '0' : value + 'A' - 10;
}
/*----------------------------------------------------------------------------*/
static void printAddress(struct Interface *serial, uint64_t address)
{
  const uint8_t * const overlay = (const uint8_t *)&address;
  uint8_t serialized[25];
  uint8_t *position = serialized;

  for (unsigned int i = 0; i < 8; ++i)
  {
    if (i > 0)
      *position++ = ' ';

    *position++ = binToHex(overlay[i] >> 4);
    *position++ = binToHex(overlay[i] & 0x0F);
  }
  *position++ = '\r';
  *position = '\n';

  ifWrite(serial, serialized, sizeof(serialized));
}
/*----------------------------------------------------------------------------*/
static void onBusEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const char separator[] = "-----------------------\r\n";

  struct Interface * const ow = init(OneWireSsp, &owConfig);
  assert(ow);

  struct Interface * const serial = init(Serial, &serialConfig);
  assert(serial);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 2000);

  bool busEvent = false;
  ifSetCallback(ow, onBusEvent, &busEvent);

  bool timerEvent = false;
  timerSetCallback(timer, onTimerOverflow, &timerEvent);
  timerEnable(timer);

  while (1)
  {
    while (!timerEvent)
      barrier();
    timerEvent = false;

    enum Result res;

    res = ifSetParam(ow, IF_ONE_WIRE_START_SEARCH, 0);
    assert(res == E_OK);

    do
    {
      while (!busEvent)
        barrier();
      busEvent = false;

      res = ifGetParam(ow, IF_STATUS, 0);
      if (res != E_OK)
        break;

      uint64_t address;

      res = ifGetParam(ow, IF_ADDRESS, &address);
      assert(res == E_OK);

      printAddress(serial, address);
    }
    while (ifSetParam(ow, IF_ONE_WIRE_FIND_NEXT, 0) == E_OK);

    ifWrite(serial, separator, sizeof(separator) - 1);
  }

  return 0;
}
