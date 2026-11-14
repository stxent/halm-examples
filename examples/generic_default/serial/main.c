/*
 * generic_default/serial/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/delay.h>
#include <halm/platform/generic/serial.h>
#include <check.h>
#include <uv.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#define DEFAULT_PORT     "/tmp/ttySerialTest"
#define INCORRECT_PORT   "/tmp/ttyIncorrectTest"

#define MAX_ITERATIONS   10
#define MESSAGE_SIZE     16
#define PORT_RATE        19200
#define RX_QUEUE_SIZE    2048
#define DRAIN_CHUNK      64
#define DRAIN_WAIT_MS    50
#define FLOOD_SIZE       (RX_QUEUE_SIZE + 64)
#define FLOOD_WAIT_MS    (1000 * FLOOD_SIZE / (PORT_RATE / 10))
#define RX_WAIT_MS       300
/*----------------------------------------------------------------------------*/
static const char *serialPortName = nullptr;
/*----------------------------------------------------------------------------*/
static void buildMessage(char *, uint32_t);
static void closeSerialPort(struct Interface *);
static void drainBuffer(struct Interface *);
static void onSerialData(void *);
static struct Interface *openSerialPort(const char *);
static struct Interface *openSerialPortConfig(const char *, enum SerialParity);
static const char *resolvePortName(int, char *[]);
static bool waitForData(struct Interface *, size_t, uint64_t);
/*----------------------------------------------------------------------------*/
static void buildMessage(char *buffer, uint32_t iteration)
{
  sprintf(buffer, "Test %09u\r\n", (unsigned int)iteration);
}
/*----------------------------------------------------------------------------*/
static void closeSerialPort(struct Interface *serial)
{
  deinit(serial);

  /* Process the pending uv_close callback of the poll handle */
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
/*----------------------------------------------------------------------------*/
static void drainBuffer(struct Interface *serial)
{
  uv_loop_t * const loop = uv_default_loop();
  const uint64_t deadline = uv_now(loop) + DRAIN_WAIT_MS;

  while (uv_now(loop) < deadline)
  {
    char buffer[DRAIN_CHUNK];

    uv_run(loop, UV_RUN_NOWAIT);

    if (!ifRead(serial, buffer, sizeof(buffer)))
      udelay(1000);
  }
}
/*----------------------------------------------------------------------------*/
static void onSerialData(void *argument)
{
  *(int *)argument += 1;
}
/*----------------------------------------------------------------------------*/
static struct Interface *openSerialPort(const char *device)
{
  return openSerialPortConfig(device, SERIAL_PARITY_NONE);
}
/*----------------------------------------------------------------------------*/
static struct Interface *openSerialPortConfig(const char *device,
    enum SerialParity parity)
{
  const struct SerialConfig config = {
      .device = device,
      .rate = PORT_RATE,
      .parity = parity
  };
  struct Interface * const serial = init(Serial, &config);

  ck_assert_ptr_nonnull(serial);
  return serial;
}
/*----------------------------------------------------------------------------*/
static const char *resolvePortName(int argc, char *argv[])
{
  return argc > 1 ? argv[1] : DEFAULT_PORT;
}
/*----------------------------------------------------------------------------*/
static bool waitForData(struct Interface *serial, size_t count, uint64_t ms)
{
  uv_loop_t * const loop = uv_default_loop();
  const uint64_t deadline = uv_now(loop) + ms;
  size_t available = 0;

  do
  {
    uv_run(loop, UV_RUN_NOWAIT);

    ck_assert_uint_eq(ifGetParam(serial, IF_RX_AVAILABLE, &available), E_OK);
    if (available >= count)
      return true;

    udelay(1000);
  }
  while (uv_now(loop) < deadline);

  uv_run(loop, UV_RUN_NOWAIT);
  ck_assert_uint_eq(ifGetParam(serial, IF_RX_AVAILABLE, &available), E_OK);

  return available >= count;
}
/*----------------------------------------------------------------------------*/
START_TEST(testCallbackNotification)
{
  struct Interface * const serial = openSerialPort(serialPortName);
  char rxBuffer[MESSAGE_SIZE * 2];
  char txBuffer[MESSAGE_SIZE + 1];
  int argument = 0;

  ifSetCallback(serial, onSerialData, &argument);

  buildMessage(txBuffer, 0);
  ck_assert_uint_eq(ifWrite(serial, txBuffer, MESSAGE_SIZE), MESSAGE_SIZE);
  ck_assert(waitForData(serial, MESSAGE_SIZE, RX_WAIT_MS));
  ck_assert_uint_ge(argument, 1);
  ck_assert_uint_eq(ifRead(serial, rxBuffer, MESSAGE_SIZE), MESSAGE_SIZE);
  ck_assert_mem_eq(rxBuffer, txBuffer, MESSAGE_SIZE);

  /* Reception must continue after the callback has been removed */
  ifSetCallback(serial, nullptr, nullptr);
  const int executions = argument;

  buildMessage(txBuffer, 1);
  ck_assert_uint_eq(ifWrite(serial, txBuffer, MESSAGE_SIZE), MESSAGE_SIZE);
  waitForData(serial, SIZE_MAX, DRAIN_WAIT_MS);

  ck_assert_uint_eq(argument, executions);
  ck_assert(waitForData(serial, MESSAGE_SIZE, RX_WAIT_MS));
  ck_assert_uint_eq(ifRead(serial, rxBuffer, MESSAGE_SIZE), MESSAGE_SIZE);
  ck_assert_mem_eq(rxBuffer, txBuffer, MESSAGE_SIZE);

  drainBuffer(serial);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testDefaultPortResolution)
{
  static char *argsCustom[] = {"serial", "/dev/ttyS0"};
  static char *argsNone[] = {"serial"};

  ck_assert_str_eq(resolvePortName(2, argsCustom), "/dev/ttyS0");
  ck_assert_str_eq(resolvePortName(1, argsNone), DEFAULT_PORT);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testInitFailure)
{
  const struct SerialConfig missingDeviceConfig = {
      .device = INCORRECT_PORT,
      .rate = PORT_RATE,
      .parity = SERIAL_PARITY_NONE
  };

  ck_assert_ptr_null(init(Serial, &missingDeviceConfig));

  const struct SerialConfig nullDeviceConfig = {
      .device = nullptr,
      .rate = PORT_RATE,
      .parity = SERIAL_PARITY_NONE
  };

  ck_assert_ptr_null(init(Serial, &nullDeviceConfig));
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testModeLineParameters)
{
  static const uint8_t enabled = 1;

  struct Interface * const serial = openSerialPort(serialPortName);
  uint32_t buffer = 0;
  size_t count = 0;
  uint8_t state = 0;

  /* Modem control ioctls are unsupported by pty devices */
  ck_assert_uint_eq(ifGetParam(serial, IF_SERIAL_CTS, &state), E_INTERFACE);
  ck_assert_uint_eq(ifGetParam(serial, IF_SERIAL_RTS, &state), E_INTERFACE);
  ck_assert_uint_eq(ifGetParam(serial, IF_SERIAL_DSR, &state), E_INTERFACE);
  ck_assert_uint_eq(ifGetParam(serial, IF_SERIAL_DTR, &state), E_INTERFACE);
  ck_assert_uint_eq(ifSetParam(serial, IF_SERIAL_RTS, &enabled), E_INTERFACE);
  ck_assert_uint_eq(ifSetParam(serial, IF_SERIAL_DTR, &enabled), E_INTERFACE);

  ck_assert_uint_eq(ifGetParam(serial, IF_TX_AVAILABLE, &count), E_OK);
  ck_assert_uint_eq(count, 0);
  ck_assert_uint_eq(ifGetParam(serial, IF_TX_PENDING, &count), E_OK);
  ck_assert_uint_eq(count, 0);

  /* Test unimplemented parameters */
  ck_assert_uint_eq(ifGetParam(serial, IF_SERIAL_FE, &buffer), E_INVALID);
  ck_assert_uint_eq(ifSetParam(serial, IF_SERIAL_FE, &buffer), E_INVALID);

  drainBuffer(serial);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testParityConfiguration)
{
  static const enum SerialParity parityList[3] = {
      SERIAL_PARITY_NONE,
      SERIAL_PARITY_ODD,
      SERIAL_PARITY_EVEN
  };

  for (size_t i = 0; i < ARRAY_SIZE(parityList); ++i)
  {
    struct Interface * const serial = openSerialPortConfig(serialPortName,
        parityList[i]);
    uint8_t parity = SERIAL_PARITY_ODD;

    ck_assert_uint_eq(ifGetParam(serial, IF_SERIAL_PARITY, &parity), E_OK);
    /* A pty device discards the parity flag and reports no parity */
    ck_assert_uint_eq(parity, SERIAL_PARITY_NONE);

    drainBuffer(serial);
    closeSerialPort(serial);
  }
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testQueueOccupancy)
{
  struct Interface * const serial = openSerialPort(serialPortName);
  char chunk[DRAIN_CHUNK];
  char buffer[FLOOD_SIZE];
  size_t available = 0;
  size_t pending = 0;
  size_t received = 0;
  size_t total = 0;

  for (size_t i = 0; i < FLOOD_SIZE; ++i)
    buffer[i] = 'a' + (char)(i % 26);

  ck_assert_uint_eq(ifWrite(serial, buffer, FLOOD_SIZE), FLOOD_SIZE);

  /* The receive queue saturates before the echo transfer completes */
  waitForData(serial, RX_QUEUE_SIZE, FLOOD_WAIT_MS);
  ck_assert_uint_eq(ifGetParam(serial, IF_RX_AVAILABLE, &available), E_OK);
  ck_assert_uint_eq(available, RX_QUEUE_SIZE);
  ck_assert_uint_eq(ifGetParam(serial, IF_RX_PENDING, &pending), E_OK);
  ck_assert_uint_eq(pending, 0);

  while ((received = ifRead(serial, chunk, sizeof(chunk))) > 0)
    total += received;

  ck_assert_uint_eq(total, RX_QUEUE_SIZE);
  ck_assert_uint_eq(ifGetParam(serial, IF_RX_PENDING, &pending), E_OK);
  ck_assert_uint_eq(pending, RX_QUEUE_SIZE);
  ck_assert_uint_eq(ifGetParam(serial, IF_RX_AVAILABLE, &available), E_OK);
  ck_assert_uint_eq(available, 0);

  drainBuffer(serial);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testRateConfiguration)
{
  static const uint32_t fastRate = 921600;
  static const uint32_t nonStandardRate = 100000;
  static const uint32_t slowRate = 9600;

  struct Interface * const serial = openSerialPort(serialPortName);
  uint32_t actualRate = 0;

  ck_assert_uint_eq(ifGetParam(serial, IF_RATE, &actualRate), E_OK);
  ck_assert_uint_eq(actualRate, PORT_RATE);

  ck_assert_uint_eq(ifSetParam(serial, IF_RATE, &fastRate), E_OK);
  ck_assert_uint_eq(ifGetParam(serial, IF_RATE, &actualRate), E_OK);
  ck_assert_uint_eq(actualRate, fastRate);

  ck_assert_uint_eq(ifSetParam(serial, IF_RATE, &slowRate), E_OK);
  ck_assert_uint_eq(ifGetParam(serial, IF_RATE, &actualRate), E_OK);
  ck_assert_uint_eq(actualRate, slowRate);

  ck_assert_uint_eq(ifSetParam(serial, IF_RATE, &nonStandardRate), E_OK);
  ck_assert_uint_eq(ifGetParam(serial, IF_RATE, &actualRate), E_OK);
  ck_assert_uint_eq(actualRate, nonStandardRate);

  drainBuffer(serial);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testRepeatedExchanges)
{
  struct Interface * const serial = openSerialPort(serialPortName);
  char rxBuffer[MESSAGE_SIZE];
  char txBuffer[MESSAGE_SIZE + 1];

  for (uint32_t iteration = 0; iteration < MAX_ITERATIONS; ++iteration)
  {
    buildMessage(txBuffer, iteration);
    ck_assert_uint_eq(ifWrite(serial, txBuffer, MESSAGE_SIZE), MESSAGE_SIZE);

    ck_assert(waitForData(serial, MESSAGE_SIZE, RX_WAIT_MS));
    ck_assert_uint_eq(ifRead(serial, rxBuffer, MESSAGE_SIZE), MESSAGE_SIZE);
    ck_assert_mem_eq(rxBuffer, txBuffer, MESSAGE_SIZE);
  }

  drainBuffer(serial);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testWriteReadEcho)
{
  struct Interface * const serial = openSerialPort(serialPortName);
  char txBuffer[MESSAGE_SIZE + 1];
  char rxBuffer[MESSAGE_SIZE];

  buildMessage(txBuffer, 0);
  ck_assert_uint_eq(ifWrite(serial, txBuffer, MESSAGE_SIZE), MESSAGE_SIZE);

  ck_assert(waitForData(serial, MESSAGE_SIZE, RX_WAIT_MS));
  ck_assert_uint_eq(ifRead(serial, rxBuffer, MESSAGE_SIZE), MESSAGE_SIZE);
  ck_assert_mem_eq(rxBuffer, txBuffer, MESSAGE_SIZE);

  drainBuffer(serial);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  serialPortName = resolvePortName(argc, argv);

  Suite * const suite = suite_create("Serial");
  TCase * const testcase = tcase_create("Core");

  tcase_add_test(testcase, testCallbackNotification);
  tcase_add_test(testcase, testDefaultPortResolution);
  tcase_add_test(testcase, testInitFailure);
  tcase_add_test(testcase, testModeLineParameters);
  tcase_add_test(testcase, testParityConfiguration);
  tcase_add_test(testcase, testQueueOccupancy);
  tcase_add_test(testcase, testRateConfiguration);
  tcase_add_test(testcase, testRepeatedExchanges);
  tcase_add_test(testcase, testWriteReadEcho);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
