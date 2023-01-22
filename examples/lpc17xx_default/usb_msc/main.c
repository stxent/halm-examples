/*
 * lpc17xx_default/usb_msc/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "interface_wrapper.h"
#include <halm/generic/mmcsd.h>
#include <halm/generic/sdio_spi.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/usb/msc.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 8192
#define TEST_DMA
#define TEST_LED
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig busyTimerConfig = {
    .frequency = 100000,
    .channel = 1
};
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const uint32_t spiSdioRate = 12000000;

  boardSetupClockPll();

  /* Helper timer */
  struct Timer * const busyTimer = init(GpTimer, &busyTimerConfig);
  assert(busyTimer);
  /* Set 5 kHz update event rate */
  timerSetOverflow(busyTimer, timerGetFrequency(busyTimer) / 5000);

  /* Initialize SPI layer */
#ifdef TEST_DMA
  struct Interface * const spi = boardSetupSpiDma();
#else
  struct Interface * const spi = boardSetupSpi();
#endif
  ifSetParam(spi, IF_RATE, &spiSdioRate);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioConfig = {
      .interface = spi,
      .timer = busyTimer,
      .wq = 0,
      .blocks = 0,
      .cs = BOARD_SDIO_CS
  };
  struct Interface * const sdio = init(SdioSpi, &sdioConfig);
  assert(sdio);

#ifdef TEST_LED
  /* Optional wrapper for R/W operations indication */
  const struct InterfaceWrapperConfig wrapperConfig = {
      .pipe = sdio,
      .rx = BOARD_LED_0,
      .tx = BOARD_LED_1
  };
  struct Interface * const wrapper = init(InterfaceWrapper, &wrapperConfig);
  assert(wrapper);
#else
  struct Interface * const wrapper = sdio;
#endif

  /* Initialize SD Card layer */
  const struct MMCSDConfig cardConfig = {
      .interface = wrapper,
      .crc = false
  };
  struct Interface * const card = init(MMCSD, &cardConfig);
  assert(card);
  ifSetParam(card, IF_ZEROCOPY, 0);

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
  assert(msc);

  mscAttachUnit(msc, 0, card);
  usbDevSetConnected(usb, true);

  while (1);
  return 0;
}
