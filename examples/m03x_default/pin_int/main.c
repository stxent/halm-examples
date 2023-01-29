/*
 * m03x_default/pin_int/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/interrupt.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
static void onExternalEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockExt();

  struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  struct Interrupt * const interrupt = boardSetupButton();
  interruptSetCallback(interrupt, onExternalEvent, &event);
  interruptEnable(interrupt);

  while (1)
  {
    while (!event)
      barrier();

    pinToggle(led);
    mdelay(10);
    event = false;
  }

  return 0;
}
