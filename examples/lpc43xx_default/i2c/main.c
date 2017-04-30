/*
 * main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <string.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/i2c.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
#define TEST_REPEATED_START
#define TEST_ZEROCOPY
/* #define TEST_SLAVE */

#ifndef TEST_SLAVE
#define DEVICE_ADDRESS      0x50
#define MEMORY_ADDRESS_SIZE 2
#else
#define DEVICE_ADDRESS      0x60
#define MEMORY_ADDRESS_SIZE 1
#endif

#define DEVICE_CLOCK  100000
#define LED_PIN       PIN(PORT_6, 6)
/*----------------------------------------------------------------------------*/
static const struct I2cConfig i2cConfig = {
    .rate = 400000, /* Initial rate */
    .scl = PIN(PORT_I2C, PIN_I2C0_SCL),
    .sda = PIN(PORT_I2C, PIN_I2C0_SDA),
    .channel = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &mainClkConfig);
}
/*----------------------------------------------------------------------------*/
enum deviceState
{
  DEVICE_IDLE,
  DEVICE_PH1_ADDRESS,
  DEVICE_PH1_DATA,
  DEVICE_PH2_DATA
};
/*----------------------------------------------------------------------------*/
struct DeviceDriver
{
  struct Interface *interface;
  struct Pin led;
  enum deviceState state;
  uint32_t desiredRate;
  uint32_t localAddress;
  uint16_t deviceAddress;
  uint8_t buffer[8 + MEMORY_ADDRESS_SIZE];

  bool change; /* Change test string */
};
/*----------------------------------------------------------------------------*/
static void deviceInit(struct DeviceDriver *device, struct Interface *interface,
    PinNumber ledNumber, uint16_t address)
{
  device->interface = interface;
  device->state = DEVICE_IDLE;
  device->desiredRate = DEVICE_CLOCK;
  device->localAddress = 0;
  device->deviceAddress = address;
  device->change = false;

  static_assert(MEMORY_ADDRESS_SIZE && MEMORY_ADDRESS_SIZE <= 2,
      "Incorrect address size");
  if (MEMORY_ADDRESS_SIZE == 2)
    device->localAddress = toBigEndian16(device->localAddress);

  device->led = pinInit(ledNumber);
  pinOutput(device->led, false);
}
/*----------------------------------------------------------------------------*/
#ifdef TEST_ZEROCOPY
static void deviceCallback(void *argument)
{
  struct DeviceDriver *device = argument;
  enum Result status;

  if ((status = ifGetParam(device->interface, IF_STATUS, 0)) != E_OK)
    return;

  switch (device->state)
  {
    case DEVICE_PH1_ADDRESS:
      device->state = DEVICE_PH1_DATA;
      ifRead(device->interface, device->buffer,
          sizeof(device->buffer) - MEMORY_ADDRESS_SIZE);
      break;

    case DEVICE_PH1_DATA:
      for (size_t i = 0; i < sizeof(device->buffer) - MEMORY_ADDRESS_SIZE; ++i)
      {
        if (device->buffer[i] != (uint8_t)(device->change ? ~i : i))
          return;
      }
      /* No break */

    case DEVICE_PH2_DATA:
      device->state = DEVICE_IDLE;
      pinReset(device->led);
      break;

    default:
      break;
  }
}
#endif
/*----------------------------------------------------------------------------*/
static void deviceConfigIO(struct DeviceDriver *device, bool rw)
{
  enum Result res;

  res = ifSetParam(device->interface, IF_RATE, &device->desiredRate);
  assert(res == E_OK);
  res = ifSetParam(device->interface, IF_ADDRESS, &device->deviceAddress);
  assert(res == E_OK);

#ifdef TEST_ZEROCOPY
  res = ifSetParam(device->interface, IF_ZEROCOPY, 0);
  assert(res == E_OK);
  res = ifSetCallback(device->interface, deviceCallback, device);
  assert(res == E_OK);
#endif

#ifdef TEST_REPEATED_START
  const bool sendStop = !rw;
#else
  const bool sendStop = true;
  (void)rw; /* Suppress warning */
#endif

  res = ifSetParam(device->interface, IF_I2C_SENDSTOP, &sendStop);
  assert(res == E_OK);

  (void)res; /* Suppress warning */
}
/*----------------------------------------------------------------------------*/
static void deviceRead(struct DeviceDriver *device)
{
  deviceConfigIO(device, true);

  pinSet(device->led);
  device->state = DEVICE_PH1_ADDRESS;

#ifdef TEST_ZEROCOPY
  ifWrite(device->interface, &device->localAddress, MEMORY_ADDRESS_SIZE);
#else
  const size_t bytesWritten = ifWrite(device->interface,
      &device->localAddress, MEMORY_ADDRESS_SIZE);

  if (bytesWritten == MEMORY_ADDRESS_SIZE)
  {
    const size_t bytesRead = ifRead(device->interface, device->buffer,
        sizeof(device->buffer) - MEMORY_ADDRESS_SIZE);

    if (bytesRead == sizeof(device->buffer) - MEMORY_ADDRESS_SIZE)
      pinReset(device->led);
  }
#endif
}
/*----------------------------------------------------------------------------*/
static void deviceWrite(struct DeviceDriver *device)
{
  deviceConfigIO(device, false);

  device->change = !device->change;
  memcpy(device->buffer, &device->localAddress, MEMORY_ADDRESS_SIZE);
  for (size_t i = 0; i < sizeof(device->buffer) - MEMORY_ADDRESS_SIZE; ++i)
    device->buffer[i + MEMORY_ADDRESS_SIZE] = device->change ? ~i : i;

  pinSet(device->led);
  device->state = DEVICE_PH2_DATA;

#ifdef TEST_ZEROCOPY
  ifWrite(device->interface, device->buffer, sizeof(device->buffer));
#else
  const size_t bytesWritten = ifWrite(device->interface,
      device->buffer, sizeof(device->buffer));

  if (bytesWritten == sizeof(device->buffer))
    pinReset(device->led);
#endif
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct DeviceDriver device;

  setupClock();

  struct Interface * const i2c = init(I2c, &i2cConfig);
  assert(i2c);

  deviceInit(&device, i2c, LED_PIN, DEVICE_ADDRESS);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  bool event = false;
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    deviceWrite(&device);

    while (!event)
      barrier();
    event = false;

    deviceRead(&device);
  }

  return 0;
}
