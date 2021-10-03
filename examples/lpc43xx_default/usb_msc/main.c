/*
 * lpc43xx_default/usb_msc/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "interface_wrapper.h"
#include <halm/generic/mmcsd.h>
#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/sdmmc.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/usb/msc.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 16384
#define LED_R       PIN(PORT_2, 5)
#define LED_W       PIN(PORT_2, 6)

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
static const struct GenericClockConfig initialClockConfig = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};

static const struct GenericClockConfig sdClockConfig = {
    .source = CLOCK_IDIVB
};

static const struct GenericDividerConfig dividerConfig = {
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
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(DividerB, &dividerConfig);
  while (!clockReady(DividerB));

  clockEnable(SdioClock, &sdClockConfig);
  while (!clockReady(SdioClock));

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
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
  const struct MMCSDConfig cardConfig = {
      .interface = wrapper,
      .crc = true
  };
  struct Interface * const card = init(MMCSD, &cardConfig);
  assert(card);
  ifSetParam(card, IF_ZEROCOPY, 0);

  /* Initialize USB peripheral */
  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);

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

  while (1);
  return 0;
}
