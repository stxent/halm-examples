/*
 * generic_default/serial/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/platform/generic/serial.h>
#include <check.h>
#include <uv.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/*----------------------------------------------------------------------------*/
#define DEFAULT_PORT    "/tmp/ttySerialTest"
#define MESSAGE_SIZE    16
#define MAX_ITERATIONS  10
#define RX_WAIT_MS      300
#define DRAIN_WAIT_MS   50
#define DRAIN_CHUNK     64
/*----------------------------------------------------------------------------*/
static const char *testPort;
/*----------------------------------------------------------------------------*/
static const char *resolvePortName(int, char *[]);
static struct Interface *openSerialPort(const char *);
static void closeSerialPort(struct Interface *);
static bool waitForRxData(struct Interface *, size_t, uint32_t);
static void drainRx(struct Interface *);
static void buildMessage(char *, uint32_t);
/*----------------------------------------------------------------------------*/
static const char *resolvePortName(int argc, char *argv[])
{
  return argc > 1 ? argv[1] : DEFAULT_PORT;
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
static void closeSerialPort(struct Interface *serial)
{
  deinit(serial);

  /* Process the pending uv_close callback of the poll handle */
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
/*----------------------------------------------------------------------------*/
static bool waitForRxData(struct Interface *serial, size_t count,
    uint32_t timeoutMs)
{
  uv_loop_t * const loop = uv_default_loop();
  const uint64_t deadline = uv_now(loop) + (uint64_t)timeoutMs;
  size_t available = 0;

  do
  {
    uv_run(loop, UV_RUN_NOWAIT);

    ifGetParam(serial, IF_RX_AVAILABLE, &available);
    if (available >= count)
      return true;

    const struct timespec slice = {.tv_sec = 0, .tv_nsec = 200 * 1000};
    nanosleep(&slice, nullptr);
  }
  while (uv_now(loop) < deadline);

  uv_run(loop, UV_RUN_NOWAIT);
  ifGetParam(serial, IF_RX_AVAILABLE, &available);

  return available >= count;
}
/*----------------------------------------------------------------------------*/
static void drainRx(struct Interface *serial)
{
  uv_loop_t * const loop = uv_default_loop();
  const uint64_t deadline = uv_now(loop) + (uint64_t)DRAIN_WAIT_MS;
  char buffer[DRAIN_CHUNK];

  while (uv_now(loop) < deadline)
  {
    uv_run(loop, UV_RUN_NOWAIT);
    const size_t available = ifRead(serial, buffer, sizeof(buffer));

    if (available == 0)
    {
      const struct timespec slice = {.tv_sec = 0, .tv_nsec = 100 * 1000};
      nanosleep(&slice, nullptr);
    }
  }
}
/*----------------------------------------------------------------------------*/
static void buildMessage(char *buffer, uint32_t iteration)
{
  sprintf(buffer, "Test %09u\r\n", (unsigned int)iteration);
}
/*----------------------------------------------------------------------------*/
START_TEST(testPortOpenClose)
{
  struct Interface * const serial = openSerialPort(testPort);

  ck_assert_ptr_ne(serial, nullptr);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testDefaultPortResolution)
{
  char *argsNone[] = {"serial"};
  char *argsCustom[] = {"serial", "/dev/ttyS0"};

  ck_assert_str_eq(resolvePortName(1, argsNone), DEFAULT_PORT);
  ck_assert_str_eq(resolvePortName(2, argsCustom), "/dev/ttyS0");
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testWriteReadEcho)
{
  struct Interface * const serial = openSerialPort(testPort);
  char txBuffer[MESSAGE_SIZE + 1];
  char rxBuffer[MESSAGE_SIZE];

  buildMessage(txBuffer, 0);
  ck_assert_uint_eq(ifWrite(serial, txBuffer, MESSAGE_SIZE), MESSAGE_SIZE);

  ck_assert(waitForRxData(serial, MESSAGE_SIZE, RX_WAIT_MS));
  ck_assert_uint_eq(ifRead(serial, rxBuffer, MESSAGE_SIZE), MESSAGE_SIZE);
  ck_assert_mem_eq(rxBuffer, txBuffer, MESSAGE_SIZE);

  drainRx(serial);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testRepeatedExchanges)
{
  struct Interface * const serial = openSerialPort(testPort);
  char txBuffer[MESSAGE_SIZE + 1];
  char rxBuffer[MESSAGE_SIZE];

  for (uint32_t iteration = 0; iteration < MAX_ITERATIONS; ++iteration)
  {
    buildMessage(txBuffer, iteration);
    ck_assert_uint_eq(ifWrite(serial, txBuffer, MESSAGE_SIZE),
        MESSAGE_SIZE);

    ck_assert(waitForRxData(serial, MESSAGE_SIZE, RX_WAIT_MS));
    ck_assert_uint_eq(ifRead(serial, rxBuffer, MESSAGE_SIZE),
        MESSAGE_SIZE);
    ck_assert_mem_eq(rxBuffer, txBuffer, MESSAGE_SIZE);
  }

  drainRx(serial);
  closeSerialPort(serial);
}
END_TEST
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  testPort = resolvePortName(argc, argv);

  Suite * const suite = suite_create("Serial");
  TCase * const testcase = tcase_create("Core");

  tcase_add_test(testcase, testPortOpenClose);
  tcase_add_test(testcase, testDefaultPortResolution);
  tcase_add_test(testcase, testWriteReadEcho);
  tcase_add_test(testcase, testRepeatedExchanges);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
