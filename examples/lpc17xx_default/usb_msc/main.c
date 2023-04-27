/*
 * lpc17xx_default/usb_msc/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "interface_wrapper.h"
#include <halm/generic/mmcsd.h>
#include <halm/generic/sdio_spi.h>
#include <halm/generic/spi.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/usb/msc.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE     8192
#define SDIO_POLL_RATE  5000
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const uint32_t SPI_SDIO_RATE = 12000000;
  static const uint8_t SPI_SDIO_MODE = 3;
  static const bool USE_BUSY_TIMER = true;
  static const bool USE_INDICATION = true;
  static const bool USE_SPI_DMA = true;

  boardSetupClockPll();

  /* Helper timer for SDIO status polling */
  struct Timer * const timer = USE_BUSY_TIMER ? boardSetupAdcTimer() : 0;

  if (timer)
  {
    /* Set 5 kHz update event rate */
    assert(timerGetFrequency(timer) >= 10 * SDIO_POLL_RATE);
    timerSetOverflow(timer, timerGetFrequency(timer) / SDIO_POLL_RATE);
  }

  struct Interface *card;
  struct Interface *sdio;
  struct Interface *spi;
  struct Interface *wrapper;
  enum Result res;

  /* Initialize SPI layer */
  spi = USE_SPI_DMA ? boardSetupSdioSpiDma() : boardSetupSdioSpi();
  res = ifSetParam(spi, IF_RATE, &SPI_SDIO_RATE);
  assert(res == E_OK);
  res = ifSetParam(spi, IF_SPI_MODE, &SPI_SDIO_MODE);
  assert(res == E_OK);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioConfig = {
      .interface = spi,
      .timer = timer,
      .wq = NULL,
      .blocks = 0,
      .cs = BOARD_SDIO_CS
  };
  sdio = init(SdioSpi, &sdioConfig);
  assert(sdio != NULL);

  /* Optional wrapper for R/W operations indication */
  if (USE_INDICATION)
  {
    const struct InterfaceWrapperConfig wrapperConfig = {
        .pipe = sdio,
        .rx = BOARD_LED_1,
        .tx = BOARD_LED_0,
        .inversion = BOARD_LED_INV
    };
    wrapper = init(InterfaceWrapper, &wrapperConfig);
    assert(wrapper != NULL);
  }
  else
    wrapper = NULL;

  /* Initialize SD Card layer */
  const struct MMCSDConfig cardConfig = {
      .interface = wrapper,
      .crc = false
  };
  card = init(MMCSD, &cardConfig);
  assert(card != NULL);
  res = ifSetParam(card, IF_ZEROCOPY, NULL);
  assert(res == E_OK);

  /* Initialize USB peripheral */
  struct Entity * const usb = boardSetupUsb();

  /* Initialize Mass Storage Device */
  const struct MscConfig config = {
      .device = usb,

      .arena = arena,
      .size = sizeof(arena),

      .endpoints = {
          .rx = 0x02,
          .tx = 0x82
      }
  };
  struct Msc * const msc = init(Msc, &config);
  assert(msc != NULL);

  mscAttachUnit(msc, 0, card);
  usbDevSetConnected(usb, true);

  /* Suppress warning */
  (void)res;

  while (1);
  return 0;
}
