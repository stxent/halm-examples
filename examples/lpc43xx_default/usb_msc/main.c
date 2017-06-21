/*
 * lpc43xx_default/usb_msc/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/generic/sdcard.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
#include <halm/platform/nxp/sdmmc.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/msc.h>
#include "interface_wrapper.h"
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE (4 * 512)
#define LED_R       PIN(PORT_6, 6)
#define LED_W       PIN(PORT_6, 7)

#define TEST_INDICATION
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_USB, PIN_USB0_DM),
    .dp = PIN(PORT_USB, PIN_USB0_DP),
    .connect = 0,
    .vbus = PIN(PORT_USB, PIN_USB0_VBUS),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct SdmmcConfig sdioConfig = {
    .rate = 12000000,
    .clk = PIN(PORT_CLK, 0),
    .cmd = PIN(PORT_1, 6),
    .dat0 = PIN(PORT_1, 9),
    .dat1 = PIN(PORT_1, 10),
    .dat2 = PIN(PORT_1, 11),
    .dat3 = PIN(PORT_1, 12)
};
/*----------------------------------------------------------------------------*/
static const struct CommonClockConfig cardClock = {
    .source = CLOCK_IDIVB
};

static const struct CommonDividerConfig dividerConfig = {
    .source = CLOCK_PLL,
    .divisor = 2
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000,
    .bypass = false
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 3,
    .multiplier = 24
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 40
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_PLL
};

static const struct CommonClockConfig initialClock = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock()
{
  clockEnable(MainClock, &initialClock);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(DividerB, &dividerConfig);
  while (!clockReady(DividerB));

  clockEnable(SdioClock, &cardClock);
  while (!clockReady(SdioClock));

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(MainClock, &mainClkConfig);
}
/*----------------------------------------------------------------------------*/
static uint8_t transferBuffer[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  /* Initialize SDIO layer */
  struct Interface * const sdio = init(Sdmmc, &sdioConfig);
  assert(sdio);

#ifdef TEST_INDICATION
  /* Optional wrapper for R/W operations indication */
  const struct InterfaceWrapperConfig wrapperConfig = {
      .pipe = sdio,
      .rx = LED_R,
      .tx = LED_W
  };
  struct Interface * const wrapper = init(InterfaceWrapper, &wrapperConfig);
  assert(wrapper);
#else
  struct Interface * const wrapper = sdio;
#endif

  /* Initialize SD Card layer */
  const struct SdCardConfig cardConfig = {
      .interface = wrapper,
      .crc = true
  };
  struct Interface * const card = init(SdCard, &cardConfig);
  assert(card);
  ifSetParam(card, IF_ZEROCOPY, 0);

  uint64_t cardSize;
  ifGetParam(card, IF_SIZE, &cardSize);

  /* Initialize USB peripheral */
  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);

  /* Initialize Mass Storage Device */
  const struct MscConfig config = {
      .device = usb,
      .storage = card,

      .buffer = transferBuffer,
      .size = sizeof(transferBuffer),

      .endpoints = {
          .rx = 0x01,
          .tx = 0x81
      }
  };
  struct Msc * const msc = init(Msc, &config);
  assert(msc);

  usbDevSetConnected(usb, true);

  while (1);

  return 0;
}
/*----------------------------------------------------------------------------*/
void __assert_func(const char *file __attribute__((unused)),
    int line __attribute__((unused)),
    const char *func __attribute__((unused)),
    const char *expr __attribute__((unused)))
{
  while (1);
}
