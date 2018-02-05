/*
 * lpc17xx_default/usb_dfu/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <halm/core/cortex/nvic.h>
#include <halm/delay.h>
#include <halm/generic/work_queue.h>
#include <halm/pin.h>
#include <halm/platform/nxp/flash.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/dfu.h>
#include <xcore/bits.h>
#include <xcore/core/cortex/asm.h>
/*----------------------------------------------------------------------------*/
#define EVENT_PIN       PIN(1, 19)

#define FIRMWARE_OFFSET 0x8000
#define FLASH_TIMEOUT   100

#define WORK_QUEUE_SIZE 4
/*----------------------------------------------------------------------------*/
struct FlashGeometry
{
  size_t number;
  size_t size;
};
/*----------------------------------------------------------------------------*/
struct FlashLoader
{
  struct Dfu *driver;
  struct Interface *flash;

  uint8_t *chunk;

  size_t flashSize;
  size_t pageSize;

  size_t bufferSize;
  size_t currentPosition;

  size_t erasingPosition;
  bool eraseQueued;
};
/*----------------------------------------------------------------------------*/
static enum Result flashLoaderInit(struct FlashLoader *, struct Interface *,
    struct Dfu *);
static void flashLoaderReset(struct FlashLoader *);
static void flashProgramTask(void *);
static bool isSectorAddress(struct FlashLoader *, size_t);
static size_t onDownloadRequest(size_t, const void *, size_t, uint16_t *);
static size_t onUploadRequest(size_t, void *, size_t);
static void setupClock(void);
static void startFirmware(void);
/*----------------------------------------------------------------------------*/
static struct FlashLoader flashLoader;
/*----------------------------------------------------------------------------*/
static const struct FlashGeometry geometry[] = {
    {
        .number = 16,
        .size = 4096
    },
    {
        .number = 14,
        .size = 32768
    }
};
/*----------------------------------------------------------------------------*/
static struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
    .vbus = PIN(1, 30),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 32
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static enum Result flashLoaderInit(struct FlashLoader *loader,
    struct Interface *flash, struct Dfu *driver)
{
  enum Result res;

  loader->driver = driver;
  loader->flash = flash;

  res = ifGetParam(loader->flash, IF_SIZE, &loader->flashSize);
  if (res != E_OK)
    return res;
  res = ifGetParam(loader->flash, IF_FLASH_PAGE_SIZE, &loader->pageSize);
  if (res != E_OK)
    return res;

  loader->chunk = malloc(loader->pageSize);
  if (!loader->chunk)
    return E_MEMORY;

  flashLoaderReset(loader);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void flashLoaderReset(struct FlashLoader *loader)
{
  loader->bufferSize = 0;
  loader->currentPosition = FIRMWARE_OFFSET;
  loader->erasingPosition = 0;
  loader->eraseQueued = false;
  memset(loader->chunk, 0xFF, loader->pageSize);
}
/*----------------------------------------------------------------------------*/
static void flashProgramTask(void *argument)
{
  struct FlashLoader * const loader = argument;

  /* Process DFU_GETSTATUS request in the USB interrupt */
  mdelay(2);

  const IrqState state = irqSave();

  loader->eraseQueued = false;
  ifSetParam(loader->flash, IF_FLASH_ERASE_SECTOR, &loader->erasingPosition);
  dfuOnDownloadCompleted(loader->driver, true);

  irqRestore(state);
}
/*----------------------------------------------------------------------------*/
static bool isSectorAddress(struct FlashLoader *loader, size_t address)
{
  if (address >= loader->flashSize)
    return false;

  size_t offset = 0;

  for (size_t index = 0; index < ARRAY_SIZE(geometry); ++index)
  {
    if (!((address - offset) % geometry[index].size))
      return true;

    offset = geometry[index].number * geometry[index].size;
  }

  return false;
}
/*----------------------------------------------------------------------------*/
static size_t onDownloadRequest(size_t position, const void *buffer,
    size_t length, uint16_t *timeout)
{
  struct FlashLoader * const loader = &flashLoader;

  if (!position)
  {
    /* Reset position and erase first sector */
    flashLoaderReset(loader);
    loader->erasingPosition = loader->currentPosition;
    loader->eraseQueued = true;
    workQueueAdd(flashProgramTask, loader);
  }

  if (loader->currentPosition + length > loader->flashSize)
    return 0;

  size_t processed = 0;

  do
  {
    if (!length || loader->bufferSize == loader->pageSize)
    {
      const enum Result res = ifSetParam(loader->flash, IF_POSITION,
          &loader->currentPosition);

      if (res != E_OK)
        return 0;

      const size_t written = ifWrite(loader->flash, loader->chunk,
          loader->pageSize);

      if (written != loader->pageSize)
        return 0;

      loader->currentPosition += loader->bufferSize;
      loader->bufferSize = 0;
      memset(loader->chunk, 0xFF, loader->pageSize);

      if (isSectorAddress(loader, loader->currentPosition))
      {
        /* Enqueue sector erasure */
        loader->erasingPosition = loader->currentPosition;
        loader->eraseQueued = true;
        workQueueAdd(flashProgramTask, loader);
      }
    }

    const size_t chunkSize = length <= loader->pageSize - loader->bufferSize ?
        length : loader->pageSize - loader->bufferSize;

    memcpy(loader->chunk + loader->bufferSize,
        (const uint8_t *)buffer + processed, chunkSize);

    loader->bufferSize += chunkSize;
    processed += chunkSize;
  }
  while (processed < length);

  *timeout = loader->eraseQueued ? FLASH_TIMEOUT : 0;

  return length;
}
/*----------------------------------------------------------------------------*/
static size_t onUploadRequest(size_t position, void *buffer, size_t length)
{
  struct FlashLoader * const loader = &flashLoader;
  const size_t offset = position + FIRMWARE_OFFSET;

  if (offset + length > loader->flashSize)
    return 0;
  if (ifSetParam(loader->flash, IF_POSITION, &offset) != E_OK)
    return 0;

  return ifRead(loader->flash, buffer, length);
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfig);

  clockEnable(UsbClock, &mainClockConfig);
  while (!clockReady(UsbClock));
}
/*----------------------------------------------------------------------------*/
static void startFirmware(void)
{
  const uint32_t * const vectorTable = (const uint32_t *)FIRMWARE_OFFSET;
  const uint32_t stackAddress = vectorTable[0];
  void (*resetVector)(void) = (void (*)(void))vectorTable[1];

  nvicSetVectorTableOffset(FIRMWARE_OFFSET);
  __setMainStackPointer(stackAddress);
  resetVector();
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin bootPin = pinInit(EVENT_PIN);
  pinInput(bootPin);
  pinSetPull(bootPin, PIN_PULLUP);

  if (pinRead(bootPin))
    startFirmware();

  setupClock();

  struct Interface * const flash = init(Flash, 0);
  assert(flash);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);

  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);

  const struct DfuConfig dfuConfig = {
      .device = usb,
      .timer = timer,
      .transferSize = 128
  };
  struct Dfu * const dfu = init(Dfu, &dfuConfig);
  assert(dfu);

  const enum Result res = flashLoaderInit(&flashLoader, flash, dfu);
  assert(res == E_OK);
  (void)res;

  dfuSetDownloadRequestCallback(dfu, onDownloadRequest);
  dfuSetUploadRequestCallback(dfu, onUploadRequest);

  usbDevSetConnected(usb, true);

  /* Initialize and start Work Queue */
  workQueueInit(WORK_QUEUE_SIZE);
  workQueueStart(0);

  return 0;
}
