/*
 * generic_default/caching_proxy/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/caching_proxy.h>
#include <halm/generic/ram_proxy.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define CACHE_BLOCKS  4
#define NUM_SECTORS   64
#define SECTOR_SIZE   512
/*----------------------------------------------------------------------------*/
static uint8_t arena[CACHE_BLOCKS * (SECTOR_SIZE + 64)];
static unsigned int failures;
/*----------------------------------------------------------------------------*/
static void fillSector(uint8_t *buffer, uint32_t pattern);
static bool sectorMatches(const uint8_t *buffer, const uint8_t *reference);
static size_t readSector(void *interface, uint32_t sector, uint8_t *buffer);
static size_t writeSector(void *interface, uint32_t sector,
    const uint8_t *buffer);
static uint32_t getCounter(void *interface, int parameter);
static void check(bool condition, const char *name);
/*----------------------------------------------------------------------------*/
static void fillSector(uint8_t *buffer, uint32_t pattern)
{
  for (size_t index = 0; index < SECTOR_SIZE; ++index)
    buffer[index] = (uint8_t)(pattern * 7 + index);
}
/*----------------------------------------------------------------------------*/
static bool sectorMatches(const uint8_t *buffer, const uint8_t *reference)
{
  return memcmp(buffer, reference, SECTOR_SIZE) == 0;
}
/*----------------------------------------------------------------------------*/
static size_t readSector(void *interface, uint32_t sector, uint8_t *buffer)
{
  const uint32_t position = sector * SECTOR_SIZE;
  [[maybe_unused]] const enum Result res =
      ifSetParam(interface, IF_POSITION, &position);

  assert(res == E_OK);
  return ifRead(interface, buffer, SECTOR_SIZE);
}
/*----------------------------------------------------------------------------*/
static size_t writeSector(void *interface, uint32_t sector,
    const uint8_t *buffer)
{
  const uint32_t position = sector * SECTOR_SIZE;
  [[maybe_unused]] const enum Result res =
      ifSetParam(interface, IF_POSITION, &position);

  assert(res == E_OK);
  return ifWrite(interface, buffer, SECTOR_SIZE);
}
/*----------------------------------------------------------------------------*/
static uint32_t getCounter(void *interface, int parameter)
{
  uint32_t value = 0;
  [[maybe_unused]] const enum Result res =
      ifGetParam(interface, parameter, &value);

  assert(res == E_OK);
  return value;
}
/*----------------------------------------------------------------------------*/
static void check(bool condition, const char *name)
{
  if (!condition)
  {
    printf("FAIL %s\r\n", name);
    ++failures;
  }
  else
    printf("PASS %s\r\n", name);
}
/*----------------------------------------------------------------------------*/
int main(int, char *[])
{
  uint8_t buffer[SECTOR_SIZE];
  uint8_t volume[NUM_SECTORS * SECTOR_SIZE];
  uint8_t written[SECTOR_SIZE];
  enum Result res;

  printf("Caching proxy test\r\n");

  for (uint32_t sector = 0; sector < NUM_SECTORS; ++sector)
    fillSector(volume + sector * SECTOR_SIZE, sector);

  /* Underlying interface: RAM volume with 512 byte sectors */
  const struct RamProxyConfig ramConfig = {
      .arena = volume,
      .capacity = sizeof(volume),
      .granule = SECTOR_SIZE
  };
  struct RamProxy * const ram = init(RamProxy, &ramConfig);
  assert(ram != nullptr);

  /* Caching proxy: 4 sectors of 512 bytes, buffer on the heap */
  const struct CachingProxyConfig cacheConfig = {
      .cache = CACHE_BLOCKS * SECTOR_SIZE,
      .size = SECTOR_SIZE,
      .arena = nullptr,
      .pipe = ram
  };
  struct CachingProxy * const cache = init(CachingProxy, &cacheConfig);
  assert(cache != nullptr);

  /* Read caching: first access is a miss, repeated access is a hit */
  check(readSector(cache, 3, buffer) == SECTOR_SIZE, "read sector 3");
  check(sectorMatches(buffer, volume + 3 * SECTOR_SIZE), "sector 3 data");
  check(getCounter(cache, IF_CACHE_READ_MISS) == 1, "read miss counter");
  check(getCounter(cache, IF_CACHE_READ_MATCH) == 0, "no read hits yet");

  check(readSector(cache, 3, buffer) == SECTOR_SIZE, "re-read sector 3");
  check(sectorMatches(buffer, volume + 3 * SECTOR_SIZE), "sector 3 data");
  check(getCounter(cache, IF_CACHE_READ_MATCH) == 1, "read hit counter");
  check(getCounter(cache, IF_CACHE_READ_MISS) == 1, "single read miss");
  printf("Read caching: %" PRIu32 " hits, %" PRIu32 " misses\r\n",
      getCounter(cache, IF_CACHE_READ_MATCH),
      getCounter(cache, IF_CACHE_READ_MISS));

  /* Write caching: dirty sector stays in the cache until the flush */
  fillSector(written, 0x100);
  check(writeSector(cache, 5, written) == SECTOR_SIZE, "write sector 5");
  check(getCounter(cache, IF_CACHE_WRITE_MISS) == 1, "write miss counter");
  check(!sectorMatches(volume + 5 * SECTOR_SIZE, written),
      "dirty sector 5 stays in the cache");

  fillSector(written, 0x101);
  check(writeSector(cache, 5, written) == SECTOR_SIZE, "re-write sector 5");
  check(getCounter(cache, IF_CACHE_WRITE_MATCH) == 1, "write hit counter");

  res = ifSetParam(cache, IF_FLUSH, nullptr);
  check(res == E_OK || res == E_INVALID, "IF_FLUSH accepted");
  check(sectorMatches(volume + 5 * SECTOR_SIZE, written),
      "sector 5 written back after IF_FLUSH");

  /* LRU eviction: a dirty block is written back on eviction */
  uint8_t sector6[SECTOR_SIZE];
  fillSector(sector6, 0x102);
  check(writeSector(cache, 6, sector6) == SECTOR_SIZE, "write sector 6");

  fillSector(written, 0x103);
  check(writeSector(cache, 7, written) == SECTOR_SIZE, "write sector 7");
  fillSector(written, 0x104);
  check(writeSector(cache, 8, written) == SECTOR_SIZE, "write sector 8");
  fillSector(written, 0x105);
  check(writeSector(cache, 9, written) == SECTOR_SIZE, "write sector 9");
  fillSector(written, 0x106);
  check(writeSector(cache, 10, written) == SECTOR_SIZE, "write sector 10");
  check(sectorMatches(volume + 6 * SECTOR_SIZE, sector6),
      "dirty sector 6 written back on eviction");

  uint8_t original7[SECTOR_SIZE];
  fillSector(original7, 7);
  check(sectorMatches(volume + 7 * SECTOR_SIZE, original7),
      "sector 7 still cached, LRU eviction order");

  /* Cached data is readable after the write */
  check(readSector(cache, 6, buffer) == SECTOR_SIZE, "read sector 6");
  check(sectorMatches(buffer, sector6), "sector 6 data from the cache");
  printf("Write caching: %" PRIu32 " hits, %" PRIu32 " misses\r\n",
      getCounter(cache, IF_CACHE_WRITE_MATCH),
      getCounter(cache, IF_CACHE_WRITE_MISS));

  /* Multi sector read bypasses the cache and invalidates the blocks */
  const uint32_t position = 10 * SECTOR_SIZE;
  uint8_t multi[2 * SECTOR_SIZE];

  check(ifSetParam(cache, IF_POSITION, &position) == E_OK, "position");
  check(ifRead(cache, multi, sizeof(multi)) == sizeof(multi),
      "multi sector read");
  check(sectorMatches(multi, volume + 10 * SECTOR_SIZE), "sector 10 data");
  check(sectorMatches(multi + SECTOR_SIZE, volume + 11 * SECTOR_SIZE),
      "sector 11 data");

  const uint32_t misses = getCounter(cache, IF_CACHE_READ_MISS);
  check(readSector(cache, 10, buffer) == SECTOR_SIZE, "re-read sector 10");
  check(getCounter(cache, IF_CACHE_READ_MISS) == misses + 1,
      "multi sector read invalidated the block");

  deinit(cache);

  /* Same test with the cache buffer taken from a memory arena */
  const struct CachingProxyConfig arenaConfig = {
      .cache = CACHE_BLOCKS * SECTOR_SIZE,
      .size = SECTOR_SIZE,
      .arena = arena,
      .pipe = ram
  };
  struct CachingProxy * const arenaCache = init(CachingProxy, &arenaConfig);
  assert(arenaCache != nullptr);

  check(readSector(arenaCache, 12, buffer) == SECTOR_SIZE, "arena read");
  check(sectorMatches(buffer, volume + 12 * SECTOR_SIZE), "sector 12 data");

  fillSector(written, 0x200);
  check(writeSector(arenaCache, 12, written) == SECTOR_SIZE, "arena write");
  check(!sectorMatches(volume + 12 * SECTOR_SIZE, written),
      "arena dirty sector stays in the cache");

  res = ifSetParam(arenaCache, IF_FLUSH, nullptr);
  check(res == E_OK || res == E_INVALID, "arena IF_FLUSH");
  check(sectorMatches(volume + 12 * SECTOR_SIZE, written),
      "arena sector 12 written back after IF_FLUSH");

  deinit(arenaCache);
  deinit(ram);

  if (failures)
  {
    printf("%u checks failed\r\n", failures);
    return EXIT_FAILURE;
  }
  else
  {
    printf("All caching proxy tests passed\r\n");
    return EXIT_SUCCESS;
  }
}
