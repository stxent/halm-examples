/*
 * lpc17xx_default/adc_dma/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/timer.h>
#include <xcore/interface.h>
#include <xcore/memory.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
#define ADC_RATE 10
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const size_t count = boardGetAdcPinCount();
  bool event = false;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Interface * const adc = boardSetupAdcDma();
  ifSetCallback(adc, onConversionCompleted, &event);
  ifSetParam(adc, IF_ENABLE, NULL);

  struct Interface * const serial = boardSetupSerial();
  struct Timer * const timer = boardSetupAdcTimer();

  /*
   * The overflow frequency of the timer should be two times higher
   * than that of the hardware events for ADC.
   */
  timerSetOverflow(timer, timerGetFrequency(timer) / (count * ADC_RATE * 2));
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    uint16_t buffer[count];
    char text[count * 6 + 3];
    char *iter = text;

    ifRead(adc, buffer, sizeof(buffer));

    for (size_t index = 0; index < count; ++index)
      iter += sprintf(iter, "%5u ", (unsigned int)buffer[index]);
    iter += sprintf(iter, "\r\n");
    ifWrite(serial, text, iter - text);

    pinToggle(led);
  }

  return 0;
}
