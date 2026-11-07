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
#define DEFAULT_PORT    "/tmp/ttySerialTest"
#define MAX_ITERATIONS  10
#define MESSAGE_SIZE    16
#define RX_WAIT_MS      300
#define DRAIN_CHUNK     64
#define DRAIN_WAIT_MS   50
/*----------------------------------------------------------------------------*/
static const char *serialPortName = nullptr;
/*----------------------------------------------------------------------------*/
static void buildMessage(char *, uint32_t);
static void closeSerialPort(struct Interface *);
static void drainBuffer(struct Interface *);
static struct Interface *openSerialPort(const char *);
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
static struct Interface *openSerialPort(const char *device)
{
  const struct SerialConfig config = {
      .device = device,
      .rate = 19200,
      .parity = SERIAL_PARITY_NONE
  };

  struct Interface * const serial = init(Serial, &config);
  ck_assert_msg(serial != nullptr, "Cannot open serial port \"%s\"", device);

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
START_TEST(testDefaultPortResolution)
{
  static char *argsCustom[] = {"serial", "/dev/ttyS0"};
  static char *argsNone[] = {"serial"};

  ck_assert_str_eq(resolvePortName(2, argsCustom), "/dev/ttyS0");
  ck_assert_str_eq(resolvePortName(1, argsNone), DEFAULT_PORT);
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

  tcase_add_test(testcase, testDefaultPortResolution);
  tcase_add_test(testcase, testRepeatedExchanges);
  tcase_add_test(testcase, testWriteReadEcho);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
