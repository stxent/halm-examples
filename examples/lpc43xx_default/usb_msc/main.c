/*
 * lpc43xx_default/usb_msc/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "interface_wrapper.h"
#include <halm/generic/mmcsd.h>
#include <halm/usb/msc.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 16384
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const uint32_t SDMMC_RATE = 17000000;
  static const bool USE_HS_USB = true;
  static const bool USE_INDICATION = true;

  boardSetupClockPll();

  struct Interface *card;
  struct Interface *sdmmc;
  struct Interface *wrapper;
  enum Result res;

  /* Initialize SDMMC layer */
  sdmmc = boardSetupSdmmc(true);
  res = ifSetParam(sdmmc, IF_RATE, &SDMMC_RATE);
  assert(res == E_OK);

  /* Optional wrapper for R/W operations indication */
  if (USE_INDICATION)
  {
    const struct InterfaceWrapperConfig wrapperConfig = {
        .pipe = sdmmc,
        .rx = BOARD_LED_0,
        .tx = BOARD_LED_1
    };
    wrapper = init(InterfaceWrapper, &wrapperConfig);
    assert(wrapper);
  }
  else
    wrapper = 0;

  /* Initialize SD Card layer */
  const struct MMCSDConfig cardConfig = {
      .interface = wrapper,
      .crc = true
  };
  card = init(MMCSD, &cardConfig);
  assert(card);
  res = ifSetParam(card, IF_ZEROCOPY, 0);
  assert(res == E_OK);

  /* Initialize USB peripheral */
  struct Entity * const usb = USE_HS_USB ? boardSetupUsb0() : boardSetupUsb1();

  /* Initialize Mass Storage Device */
  const struct MscConfig config = {
      .device = usb,

      .arena = arena,
      .size = sizeof(arena),

      .endpoints = {
          .rx = 0x01,
          .tx = 0x81
      }
  };
  struct Msc * const msc = init(Msc, &config);
  assert(msc);

  mscAttachUnit(msc, 0, card);
  usbDevSetConnected(usb, true);

  /* Suppress warning */
  (void)res;

  while (1);
  return 0;
}
