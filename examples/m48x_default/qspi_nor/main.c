/*
 * m48x_default/qspi_nor/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/qspi.h>
#include <halm/timer.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
struct DeviceInfo
{
  uint8_t manufacturer;
  uint8_t device;
} __attribute__((packed));

#define MANUFACTURER_WINBOND        0xEF

#define DEVICE_W25Q32JV             0x15
#define DEVICE_W25Q64JV             0x16
#define DEVICE_W25Q128JV            0x17
#define DEVICE_W25Q256JV            0x18

#define CMD_READ_DEVICE_ID          0x90
#define CMD_READ_DEVICE_ID_DUAL_IO  0x92
#define CMD_READ_DEVICE_ID_QUAD_IO  0x94
/*----------------------------------------------------------------------------*/
static bool isCorrectCapacity(uint8_t value)
{
  return value == DEVICE_W25Q32JV
      || value == DEVICE_W25Q64JV
      || value == DEVICE_W25Q128JV
      || value == DEVICE_W25Q256JV;
}
/*----------------------------------------------------------------------------*/
static bool isCorrectManufacturer(uint8_t value)
{
  return value == MANUFACTURER_WINBOND;
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static struct DeviceInfo readDeviceInfo(struct Interface *qspi, struct Pin cs)
{
  static const uint32_t address = 0;
  static const uint8_t command = CMD_READ_DEVICE_ID;

  struct DeviceInfo info;
  pinReset(cs);

  ifSetParam(qspi, IF_QSPI_SERIAL, 0);
  ifWrite(qspi, &command, sizeof(command));
  ifWrite(qspi, &address, 3);
  ifRead(qspi, &info, sizeof(info));

  pinSet(cs);
  return info;
}
/*----------------------------------------------------------------------------*/
static struct DeviceInfo readDeviceInfoDual(struct Interface *qspi,
    struct Pin cs)
{
  static const uint32_t address = 0;
  static const uint8_t command = CMD_READ_DEVICE_ID_DUAL_IO;
  static const uint8_t post = 0xFF;

  struct DeviceInfo info;
  pinReset(cs);

  ifSetParam(qspi, IF_QSPI_SERIAL, 0);
  ifWrite(qspi, &command, sizeof(command));

  ifSetParam(qspi, IF_QSPI_DUAL, 0);
  ifWrite(qspi, &address, 3);
  ifWrite(qspi, &post, sizeof(post));
  ifRead(qspi, &info, sizeof(info));

  pinSet(cs);
  return info;
}
/*----------------------------------------------------------------------------*/
static struct DeviceInfo readDeviceInfoQuad(struct Interface *qspi,
    struct Pin cs)
{
  static const uint32_t address = 0;
  static const uint8_t command = CMD_READ_DEVICE_ID_QUAD_IO;
  static const uint8_t delay[] = {0x00, 0x00};
  static const uint8_t post = 0xFF;

  struct DeviceInfo info;
  pinReset(cs);

  ifSetParam(qspi, IF_QSPI_SERIAL, 0);
  ifWrite(qspi, &command, sizeof(command));

  ifSetParam(qspi, IF_QSPI_QUAD, 0);
  ifWrite(qspi, &address, 3);
  ifWrite(qspi, &post, sizeof(post));
  ifWrite(qspi, delay, sizeof(delay));
  ifRead(qspi, &info, sizeof(info));

  pinSet(cs);
  return info;
}
/*----------------------------------------------------------------------------*/
static bool memoryTestSequence(struct Interface *qspi, struct Pin cs)
{
  struct DeviceInfo info;

  info = readDeviceInfo(qspi, cs);
  if (!isCorrectManufacturer(info.manufacturer))
    return false;
  if (!isCorrectCapacity(info.device))
    return false;

  info = readDeviceInfoDual(qspi, cs);
  if (!isCorrectManufacturer(info.manufacturer))
    return false;
  if (!isCorrectCapacity(info.device))
    return false;

  info = readDeviceInfoQuad(qspi, cs);
  if (!isCorrectManufacturer(info.manufacturer))
    return false;
  if (!isCorrectCapacity(info.device))
    return false;

  return true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockExt();

  const struct Pin cs = pinInit(BOARD_QSPI_CS);
  pinOutput(cs, true);
  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Interface * const qspi = boardSetupQspi();

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
    if (memoryTestSequence(qspi, cs))
      pinWrite(led, BOARD_LED_INV);
  }

  return 0;
}
