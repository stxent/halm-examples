/*
 * generic_default/caching_proxy/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/caching_proxy.h>
#include <halm/generic/flash.h>
#include <halm/generic/ram_proxy.h>
#include <check.h>
#include <stdint.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#define CACHE_BLOCKS    4
#define SECTOR_SIZE     512
#define LARGE_BLOCKS    64
#define LARGE_CAPACITY  (SECTOR_SIZE * LARGE_BLOCKS)
#define TINY_BLOCKS     2
#define TINY_CAPACITY   1100
/*----------------------------------------------------------------------------*/
static void callbackHandler(void *);
static void fillSector(uint8_t *, uint32_t);
static uint32_t getCounter(void *, int);
static void *makeVolume(size_t);
static struct CachingProxy *openCachingProxy(struct RamProxy *, size_t);
static struct RamProxy *openRamVolume(uint8_t *, size_t);
static size_t readSector(void *, uint32_t, uint8_t *);
static size_t writeSector(void *, uint32_t, const uint8_t *);
/*----------------------------------------------------------------------------*/
static void callbackHandler(void *argument)
{
  *(int *)argument += 1;
}
/*----------------------------------------------------------------------------*/
static void fillSector(uint8_t *buffer, uint32_t pattern)
{
  for (size_t index = 0; index < SECTOR_SIZE; ++index)
    buffer[index] = (uint8_t)(pattern * 7 + index);
}
/*----------------------------------------------------------------------------*/
static uint32_t getCounter(void *interface, int parameter)
{
  uint32_t value = 0;

  ck_assert_uint_eq(ifGetParam(interface, parameter, &value), E_OK);
  return value;
}
/*----------------------------------------------------------------------------*/
static void *makeVolume(size_t capacity)
{
  uint8_t * const volume = malloc(capacity);
  ck_assert_ptr_nonnull(volume);

  memset(volume, 0, capacity);
  return volume;
}
/*----------------------------------------------------------------------------*/
static struct CachingProxy *openCachingProxy(struct RamProxy *pipe,
    size_t blockCount)
{
  const struct CachingProxyConfig config = {
      .cache = blockCount * SECTOR_SIZE,
      .size = SECTOR_SIZE,
      .arena = nullptr,
      .pipe = pipe
  };

  struct CachingProxy * const cache = init(CachingProxy, &config);
  ck_assert_ptr_nonnull(cache);

  return cache;
}
/*----------------------------------------------------------------------------*/
static struct RamProxy *openRamVolume(uint8_t *arena, size_t capacity)
{
  const struct RamProxyConfig config = {
      .arena = arena,
      .capacity = capacity,
      .granule = SECTOR_SIZE
  };

  struct RamProxy * const ram = init(RamProxy, &config);
  ck_assert_ptr_nonnull(ram);

  uint64_t size64;
  uint32_t size32;

  ck_assert_uint_eq(ifGetParam(ram, IF_FLASH_SECTOR_SIZE, &size32), E_OK);
  ck_assert_uint_eq(size32, SECTOR_SIZE);
  ck_assert_uint_eq(ifGetParam(ram, IF_SIZE, &size32), E_OK);
  ck_assert_uint_eq(size32, (uint32_t)capacity);
  ck_assert_uint_eq(ifGetParam(ram, IF_SIZE_64, &size64), E_OK);
  ck_assert_uint_eq(size64, (uint64_t)capacity);

  for (size_t i = 0; i < capacity / SECTOR_SIZE; ++i)
  {
    const uint32_t address = SECTOR_SIZE * i;

    ck_assert_uint_eq(ifSetParam(ram, IF_FLASH_ERASE_SECTOR, &address), E_OK);
    ck_assert_uint_eq(ifGetParam(ram, IF_STATUS, nullptr), E_OK);
  }

  if (capacity % SECTOR_SIZE)
  {
    /* Last sector has unaligned size */
    const uint32_t address = capacity - capacity % SECTOR_SIZE;

    ck_assert_uint_ne(ifSetParam(ram, IF_FLASH_ERASE_SECTOR, &address), E_OK);
    ck_assert_uint_eq(ifGetParam(ram, IF_STATUS, nullptr), E_OK);
  }

  return ram;
}
/*----------------------------------------------------------------------------*/
static size_t readSector(void *interface, uint32_t sector, uint8_t *buffer)
{
  const uint32_t position = sector * SECTOR_SIZE;

  ck_assert_uint_eq(ifSetParam(interface, IF_POSITION, &position), E_OK);
  return ifRead(interface, buffer, SECTOR_SIZE);
}
/*----------------------------------------------------------------------------*/
static size_t writeSector(void *interface, uint32_t sector,
    const uint8_t *buffer)
{
  const uint32_t position = sector * SECTOR_SIZE;

  ck_assert_uint_eq(ifSetParam(interface, IF_POSITION, &position), E_OK);
  return ifWrite(interface, buffer, SECTOR_SIZE);
}
/*----------------------------------------------------------------------------*/
START_TEST(testArenaBuffer)
{
  uint8_t arena[CACHE_BLOCKS * (SECTOR_SIZE + 64)];
  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  uint8_t buffer[SECTOR_SIZE];

  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);

  const struct CachingProxyConfig config = {
      .cache = CACHE_BLOCKS * SECTOR_SIZE,
      .size = SECTOR_SIZE,
      .arena = arena,
      .pipe = ram
  };
  struct CachingProxy * const cache = init(CachingProxy, &config);
  ck_assert_msg(cache != nullptr, "Cannot initialize the caching proxy");

  ck_assert_uint_eq(readSector(cache, 12, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, volume + SECTOR_SIZE * 12, SECTOR_SIZE);

  fillSector(buffer, 0x11);
  ck_assert_uint_eq(writeSector(cache, 12, buffer), SECTOR_SIZE);
  ck_assert_mem_ne(volume + SECTOR_SIZE * 12, buffer, SECTOR_SIZE);

  ck_assert_uint_eq(ifSetParam(cache, IF_FLUSH, nullptr), E_OK);
  ck_assert_mem_eq(volume + SECTOR_SIZE * 12, buffer, SECTOR_SIZE);

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testCallback)
{
  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  int argument = 0;

  /* The callback is stored in the proxy */
  ifSetCallback(cache, callbackHandler, &argument);

  /* The stored callback is invoked from the callback wrapper */
  ck_assert(ram->callback != nullptr);
  ram->callback(ram->callbackArgument);
  ck_assert_int_eq(argument, 1);

  /* Reset the callback */
  ifSetCallback(cache, nullptr, nullptr);

  deinit(cache);
  deinit(ram);
}
END_TEST

/*----------------------------------------------------------------------------*/
START_TEST(testEviction)
{
  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  uint8_t sector6[SECTOR_SIZE];
  uint8_t sector7[SECTOR_SIZE];
  uint8_t buffer[SECTOR_SIZE];

  /* Fill the four block cache, the oldest dirty block is written back */
  fillSector(sector6, 0x11);
  ck_assert_uint_eq(writeSector(cache, 6, sector6), SECTOR_SIZE);

  fillSector(buffer, 0x22);
  ck_assert_uint_eq(writeSector(cache, 7, buffer), SECTOR_SIZE);
  fillSector(buffer, 0x33);
  ck_assert_uint_eq(writeSector(cache, 8, buffer), SECTOR_SIZE);
  fillSector(buffer, 0x44);
  ck_assert_uint_eq(writeSector(cache, 9, buffer), SECTOR_SIZE);

  /* The fifth write evicts the oldest block (sector 6) */
  fillSector(buffer, 0x55);
  ck_assert_uint_eq(writeSector(cache, 10, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(volume + SECTOR_SIZE * 6, sector6, SECTOR_SIZE);

  /* Sector 7 is still cached and the volume copy is untouched */
  memset(sector7, 0, SECTOR_SIZE);
  ck_assert_mem_eq(volume + SECTOR_SIZE * 7, sector7, SECTOR_SIZE);

  /* The evicted sector is readable from the volume */
  ck_assert_uint_eq(readSector(cache, 6, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, sector6, SECTOR_SIZE);

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testFaultInjection)
{
  uint8_t * const volume = makeVolume(TINY_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, TINY_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, TINY_BLOCKS);
  uint32_t position;
  uint8_t buffer[SECTOR_SIZE];

  /* A fetch that ends past the pipe capacity is rejected */
  {
    const uint32_t origin = SECTOR_SIZE * 3;

    ck_assert_uint_ne(ifSetParam(cache, IF_POSITION, &origin), E_OK);
    ck_assert_uint_eq(ifGetParam(cache, IF_POSITION, &position), E_OK);
    ck_assert_uint_eq(position, 0);
  }

  /* A fetch that extends past the pipe end reads a short block */
  {
    position = SECTOR_SIZE * 2;

    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &position), E_OK);
    ck_assert_uint_eq(ifRead(cache, buffer, SECTOR_SIZE), 0);
  }

  /* Flushing a dirty block that does not fit the pipe fails */
  {
    position = SECTOR_SIZE * 2;

    fillSector(buffer, 0x11);
    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &position), E_OK);
    ck_assert_uint_eq(ifWrite(cache, buffer, SECTOR_SIZE), SECTOR_SIZE);
    ck_assert_uint_eq(getCounter(cache, IF_CACHE_WRITE_MISS), 1);
    ck_assert_uint_ne(ifSetParam(cache, IF_FLUSH, nullptr), E_OK);
  }

  /* Simulate a flushing error during block allocation */
  {
    position = SECTOR_SIZE * 2;
    fillSector(buffer, 0x11);
    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &position), E_OK);
    ck_assert_uint_eq(ifWrite(cache, buffer, SECTOR_SIZE), SECTOR_SIZE);

    position = SECTOR_SIZE;
    fillSector(buffer, 0x22);
    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &position), E_OK);
    ck_assert_uint_eq(ifWrite(cache, buffer, SECTOR_SIZE), SECTOR_SIZE);

    position = 0;
    fillSector(buffer, 0x33);
    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &position), E_OK);
    ck_assert_uint_eq(ifWrite(cache, buffer, SECTOR_SIZE), 0);
    ck_assert_uint_eq(ifRead(cache, buffer, SECTOR_SIZE), 0);
  }

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testMultiSectorRead)
{
  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  uint8_t multi[SECTOR_SIZE * 2];
  uint8_t buffer[SECTOR_SIZE];

  /* Cache sector 10 before the multi sector read */
  ck_assert_uint_eq(readSector(cache, 10, buffer), SECTOR_SIZE);

  /* A multi sector read bypasses the cache and invalidates the range */
  const uint32_t position = SECTOR_SIZE * 10;

  ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &position), E_OK);
  ck_assert_uint_eq(ifRead(cache, multi, sizeof(multi)), sizeof(multi));
  ck_assert_mem_eq(multi, volume + SECTOR_SIZE * 10, SECTOR_SIZE);
  ck_assert_mem_eq(multi + SECTOR_SIZE, volume + SECTOR_SIZE * 11, SECTOR_SIZE);

  /* The cached block is invalidated, the re-read is a miss */
  const uint32_t misses = getCounter(cache, IF_CACHE_READ_MISS);

  ck_assert_uint_eq(readSector(cache, 10, buffer), SECTOR_SIZE);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MISS), misses + 1);

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testMultiSectorWrite)
{
  static const uint32_t alignedPosition = SECTOR_SIZE * 12;
  static const uint32_t partialPosition = SECTOR_SIZE * 14;
  static const uint32_t partialOffset = 88;

  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  uint32_t misses;
  uint8_t buffer[SECTOR_SIZE];
  uint8_t multi[SECTOR_SIZE * 2];
  uint8_t partial[SECTOR_SIZE + partialOffset];

  /* Cache sectors 12 and 13 before the multi sector write */
  ck_assert_uint_eq(readSector(cache, 12, buffer), SECTOR_SIZE);
  ck_assert_uint_eq(readSector(cache, 13, buffer), SECTOR_SIZE);

  /* A two sector write bypasses the cache and updates the volume */
  fillSector(multi, 0x11);
  fillSector(multi + SECTOR_SIZE, 0x22);
  ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &alignedPosition), E_OK);
  ck_assert_uint_eq(ifWrite(cache, multi, sizeof(multi)), sizeof(multi));
  ck_assert_mem_eq(volume + SECTOR_SIZE * 12, multi, SECTOR_SIZE);
  ck_assert_mem_eq(volume + SECTOR_SIZE * 13, multi + SECTOR_SIZE, SECTOR_SIZE);

  /* Both cached sectors are invalidated, the re-reads are misses */
  misses = getCounter(cache, IF_CACHE_READ_MISS);
  ck_assert_uint_eq(readSector(cache, 12, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, multi, SECTOR_SIZE);
  ck_assert_uint_eq(readSector(cache, 13, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, multi + SECTOR_SIZE, SECTOR_SIZE);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MISS), misses + 2);

  /* A write that is not block aligned straddles two cached sectors */
  ck_assert_uint_eq(readSector(cache, 14, buffer), SECTOR_SIZE);
  ck_assert_uint_eq(readSector(cache, 15, buffer), SECTOR_SIZE);

  memset(partial, 0x33, sizeof(partial));
  ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &partialPosition), E_OK);
  ck_assert_uint_eq(ifWrite(cache, partial, sizeof(partial)), sizeof(partial));
  ck_assert_mem_eq(volume + SECTOR_SIZE * 14, partial, SECTOR_SIZE);
  ck_assert_mem_eq(volume + SECTOR_SIZE * 15, partial + SECTOR_SIZE,
      partialOffset);

  /* Both straddled sectors are invalidated, the re-reads are misses */
  misses = getCounter(cache, IF_CACHE_READ_MISS);
  ck_assert_uint_eq(readSector(cache, 14, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, partial, SECTOR_SIZE);
  ck_assert_uint_eq(readSector(cache, 15, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, partial + SECTOR_SIZE, partialOffset);
  ck_assert_mem_eq(buffer + partialOffset,
      volume + SECTOR_SIZE * 15 + partialOffset, SECTOR_SIZE - partialOffset);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MISS), misses + 2);

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testPosition64)
{
  static const uint32_t origin = SECTOR_SIZE * LARGE_BLOCKS / 2;

  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  uint64_t position64;
  uint32_t position32;
  uint32_t size32;
  uint8_t buffer[SECTOR_SIZE];

  /* A valid 64 bit position moves both the cache and the pipe */
  position64 = origin;
  ck_assert_uint_eq(ifSetParam(cache, IF_POSITION_64, &position64), E_OK);
  position64 = 0;
  ck_assert_uint_eq(ifGetParam(cache, IF_POSITION_64, &position64), E_OK);
  ck_assert_uint_eq(position64, (uint64_t)origin);

  /* The 32 bit position still works and reports back through the pipe */
  position32 = 0;
  ck_assert_uint_eq(ifGetParam(cache, IF_POSITION, &position32), E_OK);
  ck_assert_uint_eq(position32, origin);

  ck_assert_uint_eq(readSector(cache, origin / SECTOR_SIZE, buffer),
      SECTOR_SIZE);
  ck_assert_mem_eq(buffer, volume + origin, SECTOR_SIZE);

  /* A position that does not fit the capacity is rejected */
  position64 = (uint64_t)SECTOR_SIZE * ((uint64_t)UINT32_MAX + 1);
  ck_assert_uint_ne(ifSetParam(cache, IF_POSITION_64, &position64), E_OK);

  position64 = (uint64_t)SECTOR_SIZE * LARGE_BLOCKS * 2;
  ck_assert_uint_ne(ifSetParam(cache, IF_POSITION_64, &position64), E_OK);

  position32 = SECTOR_SIZE * LARGE_BLOCKS * 2;
  ck_assert_uint_ne(ifSetParam(cache, IF_POSITION, &position32), E_OK);

  /* Unsupported parameters fall through to the pipe */
  ck_assert_uint_eq(ifGetParam(cache, IF_SIZE, &size32), E_OK);
  ck_assert_uint_eq(size32, LARGE_CAPACITY);
  ck_assert_uint_eq(ifSetParam(cache, IF_ZEROCOPY, nullptr), E_OK);
  ck_assert_uint_eq(ifSetParam(cache, IF_BLOCKING, nullptr), E_OK);

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testPositionErrors)
{
  static const uint32_t origin = SECTOR_SIZE * 2;

  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  uint8_t buffer[SECTOR_SIZE];

  /* Inject position error during read */
  {
    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &origin), E_OK);
    ram->capacity = SECTOR_SIZE;
    ck_assert_uint_eq(ifRead(cache, buffer, SECTOR_SIZE), 0);
    ram->capacity = LARGE_CAPACITY;
  }

  /* Inject position error during flush */
  {
    fillSector(buffer, 0x11);
    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &origin), E_OK);
    ck_assert_uint_eq(ifWrite(cache, buffer, SECTOR_SIZE), SECTOR_SIZE);
    ram->capacity = SECTOR_SIZE;
    ck_assert_uint_ne(ifSetParam(cache, IF_FLUSH, nullptr), E_OK);
    ram->capacity = LARGE_CAPACITY;
  }

  /* Inject position error during address restoration */
  {
    const uint32_t offset = SECTOR_SIZE * 4;

    fillSector(buffer, 0x22);
    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &origin), E_OK);
    ck_assert_uint_eq(ifWrite(cache, buffer, SECTOR_SIZE), SECTOR_SIZE);
    ck_assert_uint_eq(ifSetParam(cache, IF_POSITION, &offset), E_OK);
    ram->capacity = SECTOR_SIZE * 3;
    ck_assert_uint_ne(ifSetParam(cache, IF_FLUSH, nullptr), E_OK);
    ram->capacity = LARGE_CAPACITY;
  }

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testReadCaching)
{
  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  uint8_t buffer[SECTOR_SIZE];

  /* The first access is a cache miss and fetches the sector from the pipe */
  ck_assert_uint_eq(readSector(cache, 3, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, volume + SECTOR_SIZE * 3, SECTOR_SIZE);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MISS), 1);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MATCH), 0);

  /* A repeated access is served from the cache */
  ck_assert_uint_eq(readSector(cache, 3, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, volume + SECTOR_SIZE * 3, SECTOR_SIZE);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MATCH), 1);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MISS), 1);

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testTimestampOverflow)
{
  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  uint8_t sector20[SECTOR_SIZE];
  uint8_t sector23[SECTOR_SIZE];
  uint8_t buffer[SECTOR_SIZE];
  uint8_t zero[SECTOR_SIZE];
  uint32_t matches;
  uint32_t misses;

  /* Place the counter three increments before the wrap point */
  cache->timestamp = UINT32_MAX - 3;

  /* Four dirty blocks are cached, the counter wraps on the last write */
  fillSector(sector20, 0x11);
  ck_assert_uint_eq(writeSector(cache, 20, sector20), SECTOR_SIZE);
  fillSector(buffer, 0x22);
  ck_assert_uint_eq(writeSector(cache, 21, buffer), SECTOR_SIZE);
  fillSector(buffer, 0x33);
  ck_assert_uint_eq(writeSector(cache, 22, buffer), SECTOR_SIZE);
  fillSector(sector23, 0x44);
  ck_assert_uint_eq(writeSector(cache, 23, sector23), SECTOR_SIZE);

  /*
   * The fifth write evicts the oldest block. Sector 20 holds the pre-wrap
   * timestamp, sector 23 holds the post-wrap timestamp and is the most recent
   * block despite its small value.
   */
  fillSector(buffer, 0x55);
  ck_assert_uint_eq(writeSector(cache, 24, buffer), SECTOR_SIZE);

  /* The pre-wrap block is flushed, the post-wrap block stays cached */
  ck_assert_mem_eq(volume + SECTOR_SIZE * 20, sector20, SECTOR_SIZE);

  memset(zero, 0, SECTOR_SIZE);
  ck_assert_mem_eq(volume + SECTOR_SIZE * 23, zero, SECTOR_SIZE);

  /* The cached block is served from the cache, the flushed one is a miss */
  matches = getCounter(cache, IF_CACHE_READ_MATCH);
  misses = getCounter(cache, IF_CACHE_READ_MISS);

  ck_assert_uint_eq(readSector(cache, 23, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, sector23, SECTOR_SIZE);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MATCH), matches + 1);

  ck_assert_uint_eq(readSector(cache, 20, buffer), SECTOR_SIZE);
  ck_assert_mem_eq(buffer, sector20, SECTOR_SIZE);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_READ_MISS), misses + 1);

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
START_TEST(testWriteCaching)
{
  uint8_t * const volume = makeVolume(LARGE_CAPACITY);
  struct RamProxy * const ram = openRamVolume(volume, LARGE_CAPACITY);
  struct CachingProxy * const cache = openCachingProxy(ram, CACHE_BLOCKS);
  uint8_t buffer[SECTOR_SIZE];

  /* The first write allocates a cache block and marks it dirty */
  fillSector(buffer, 0x11);
  ck_assert_uint_eq(writeSector(cache, 5, buffer), SECTOR_SIZE);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_WRITE_MISS), 1);
  ck_assert_mem_ne(volume + SECTOR_SIZE * 5, buffer, SECTOR_SIZE);

  /* The second write updates the cached block */
  fillSector(buffer, 0x22);
  ck_assert_uint_eq(writeSector(cache, 5, buffer), SECTOR_SIZE);
  ck_assert_uint_eq(getCounter(cache, IF_CACHE_WRITE_MATCH), 1);
  ck_assert_mem_ne(volume + SECTOR_SIZE * 5, buffer, SECTOR_SIZE);

  /* The dirty block is written back to the pipe on IF_FLUSH */
  ck_assert_uint_eq(ifSetParam(cache, IF_FLUSH, nullptr), E_OK);
  ck_assert_mem_eq(volume + SECTOR_SIZE * 5, buffer, SECTOR_SIZE);

  deinit(cache);
  deinit(ram);
  free(volume);
}
END_TEST
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  Suite * const suite = suite_create("CachingProxy");
  TCase * const testcase = tcase_create("Core");

  tcase_add_test(testcase, testArenaBuffer);
  tcase_add_test(testcase, testCallback);
  tcase_add_test(testcase, testEviction);
  tcase_add_test(testcase, testFaultInjection);
  tcase_add_test(testcase, testMultiSectorRead);
  tcase_add_test(testcase, testMultiSectorWrite);
  tcase_add_test(testcase, testPosition64);
  tcase_add_test(testcase, testPositionErrors);
  tcase_add_test(testcase, testReadCaching);
  tcase_add_test(testcase, testTimestampOverflow);
  tcase_add_test(testcase, testWriteCaching);
  suite_add_tcase(suite, testcase);

  SRunner * const runner = srunner_create(suite);

  srunner_run_all(runner, CK_NORMAL);
  const int failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
