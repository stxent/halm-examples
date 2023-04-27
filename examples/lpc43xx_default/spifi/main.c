/*
 * lpc43xx_default/spifi/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/spim.h>
#include <halm/platform/lpc/spifi.h>
#include <halm/timer.h>
#include <xcore/bits.h>
#include <xcore/memory.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
struct DeviceInfo
{
  uint8_t manufacturer;
  uint8_t device;
} __attribute__((packed));

#define MANUFACTURER_WINBOND        0xEF
#define DEVICE_W25Q256JV            0x18

#define CMD_READ_DEVICE_ID          0x90
#define CMD_READ_DEVICE_ID_DUAL_IO  0x92
#define CMD_READ_DEVICE_ID_QUAD_IO  0x94
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static struct DeviceInfo readDeviceInfo(struct Interface *spifi)
{
  static const uint32_t length = TO_LITTLE_ENDIAN_32(sizeof(struct DeviceInfo));
  static const uint32_t address = 0;
  static const uint8_t command = CMD_READ_DEVICE_ID;
  struct DeviceInfo info;

  ifSetParam(spifi, IF_SPIM_COMMAND, &command);
  ifSetParam(spifi, IF_SPIM_COMMAND_SERIAL, NULL);
  ifSetParam(spifi, IF_SPIM_ADDRESS_24, &address);
  ifSetParam(spifi, IF_SPIM_ADDRESS_SERIAL, NULL);
  ifSetParam(spifi, IF_SPIM_POST_ADDRESS_NONE, NULL);
  ifSetParam(spifi, IF_SPIM_DELAY_NONE, NULL);
  ifSetParam(spifi, IF_SPIM_DATA_LENGTH, &length);
  ifSetParam(spifi, IF_SPIM_DATA_SERIAL, NULL);

  ifRead(spifi, &info, sizeof(info));
  return info;
}
/*----------------------------------------------------------------------------*/
static struct DeviceInfo readDeviceInfoDual(struct Interface *spifi)
{
  static const uint32_t length = TO_LITTLE_ENDIAN_32(sizeof(struct DeviceInfo));
  static const uint32_t address = 0;
  static const uint8_t command = CMD_READ_DEVICE_ID_DUAL_IO;
  static const uint8_t post = 0xFF;
  struct DeviceInfo info;

  ifSetParam(spifi, IF_SPIM_DUAL, NULL);
  ifSetParam(spifi, IF_SPIM_COMMAND, &command);
  ifSetParam(spifi, IF_SPIM_COMMAND_SERIAL, NULL);
  ifSetParam(spifi, IF_SPIM_ADDRESS_24, &address);
  ifSetParam(spifi, IF_SPIM_ADDRESS_PARALLEL, NULL);
  ifSetParam(spifi, IF_SPIM_POST_ADDRESS_8, &post);
  ifSetParam(spifi, IF_SPIM_POST_ADDRESS_PARALLEL, NULL);
  ifSetParam(spifi, IF_SPIM_DELAY_NONE, NULL);
  ifSetParam(spifi, IF_SPIM_DATA_LENGTH, &length);
  ifSetParam(spifi, IF_SPIM_DATA_PARALLEL, NULL);

  ifRead(spifi, &info, sizeof(info));
  return info;
}
/*----------------------------------------------------------------------------*/
static struct DeviceInfo readDeviceInfoQuad(struct Interface *spifi)
{
  static const uint32_t length = TO_LITTLE_ENDIAN_32(sizeof(struct DeviceInfo));
  static const uint32_t address = 0;
  static const uint8_t command = CMD_READ_DEVICE_ID_QUAD_IO;
  static const uint8_t delay = 2;
  static const uint8_t post = 0xFF;
  struct DeviceInfo info;

  ifSetParam(spifi, IF_SPIM_QUAD, NULL);
  ifSetParam(spifi, IF_SPIM_COMMAND, &command);
  ifSetParam(spifi, IF_SPIM_COMMAND_SERIAL, NULL);
  ifSetParam(spifi, IF_SPIM_ADDRESS_24, &address);
  ifSetParam(spifi, IF_SPIM_ADDRESS_PARALLEL, NULL);
  ifSetParam(spifi, IF_SPIM_POST_ADDRESS_8, &post);
  ifSetParam(spifi, IF_SPIM_POST_ADDRESS_PARALLEL, NULL);
  ifSetParam(spifi, IF_SPIM_DELAY_LENGTH, &delay);
  ifSetParam(spifi, IF_SPIM_DELAY_PARALLEL, NULL);
  ifSetParam(spifi, IF_SPIM_DATA_LENGTH, &length);
  ifSetParam(spifi, IF_SPIM_DATA_PARALLEL, NULL);

  ifRead(spifi, &info, sizeof(info));
  return info;
}
/*----------------------------------------------------------------------------*/
static bool memoryTestSequence(struct Interface *spifi)
{
  struct DeviceInfo info;

  info = readDeviceInfo(spifi);
  if (info.manufacturer != MANUFACTURER_WINBOND)
    return false;
  if (info.device != DEVICE_W25Q256JV)
    return false;

  info = readDeviceInfoDual(spifi);
  if (info.manufacturer != MANUFACTURER_WINBOND)
    return false;
  if (info.device != DEVICE_W25Q256JV)
    return false;

  info = readDeviceInfoQuad(spifi);
  if (info.manufacturer != MANUFACTURER_WINBOND)
    return false;
  if (info.device != DEVICE_W25Q256JV)
    return false;

  return true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockExt();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Interface * const spifi = boardSetupSpifi();

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer));
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinWrite(led, !BOARD_LED_INV);
    memoryTestSequence(spifi);
    pinWrite(led, BOARD_LED_INV);
  }

  return 0;
}
