/*
 * lpc43xx_default/emc_flash/main.c
 * Copyright (C) 2025 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include "flash_cfi.h"
#include <halm/generic/flash.h>
#include <halm/platform/lpc/emc_sram.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE     2048
#define MEMORY_CAPACITY (2 * 1024 * 1024)
#define MEMORY_OFFSET   (64 * 1024)
/*----------------------------------------------------------------------------*/
/* SST39VF3201 */
static const struct EmcSramConfig emcFlashConfig = {
    .timings = {
        .oe = 0,
        .rd = 70,
        .we = 30,
        .wr = 70
    },

    .width = {
        .address = 23,
        .data = 16
    },

    .channel = 0,
    .buffering = false,
    .useWriteEnable = true
};
/*----------------------------------------------------------------------------*/
static bool program(struct Interface *interface, uint32_t address,
    size_t length)
{
  uint32_t next = 0;
  uint32_t buffer[length / 4];

  for (size_t i = 0; i < ARRAY_SIZE(buffer); ++i)
    buffer[i] = (uint32_t)i;

  if (ifSetParam(interface, IF_POSITION, &address) != E_OK)
    return false;
  if (ifWrite(interface, buffer, sizeof(buffer)) != sizeof(buffer))
    return false;
  if (ifGetParam(interface, IF_POSITION, &next) != E_OK)
    return false;
  if (next != address + sizeof(buffer))
    return false;

  return true;
}
/*----------------------------------------------------------------------------*/
static bool verify(struct Interface *interface, uint32_t address,
    size_t length)
{
  uint32_t next = 0;
  uint32_t buffer[length / 4];

  memset(buffer, 0, sizeof(buffer));

  if (ifSetParam(interface, IF_POSITION, &address) != E_OK)
    return false;
  if (ifRead(interface, buffer, sizeof(buffer)) != sizeof(buffer))
    return false;
  if (ifGetParam(interface, IF_POSITION, &next) != E_OK)
    return false;
  if (next != address + sizeof(buffer))
    return false;

  for (size_t i = 0; i < ARRAY_SIZE(buffer); ++i)
  {
    if (buffer[i] != (uint32_t)i)
      return false;
  }

  return true;
}
/*----------------------------------------------------------------------------*/
bool memoryTestSequence(struct Interface *interface, int paramSize,
    int paramErase)
{
  const uint32_t address = MEMORY_OFFSET;
  size_t size = BUFFER_SIZE;

  uint32_t capacity;
  uint32_t chunk;

  if (ifGetParam(interface, IF_SIZE, &capacity) != E_OK)
    return false;
  if (!capacity)
    return false;

  if (ifGetParam(interface, paramSize, &chunk) == E_OK)
  {
    size = MIN(size, (size_t)chunk);

    if (ifSetParam(interface, paramErase, &address) != E_OK)
      return false;

    if (!program(interface, address, size))
      return false;

    if (!verify(interface, address, size))
      return false;
  }

  return true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockPllCustom(96000000);

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct EmcSram * const memory = init(EmcSram, &emcFlashConfig);
  assert(memory != NULL);

  const struct FlashCFIConfig config = {
      .address = emcSramAddress(memory),
      .size = MEMORY_CAPACITY
  };
  struct Interface * const flash = init(FlashCFI, &config);
  assert(flash != NULL);

  /* Test sector erase */
  if (!memoryTestSequence(flash, IF_FLASH_SECTOR_SIZE, IF_FLASH_ERASE_SECTOR))
    pinWrite(led, !BOARD_LED_INV);

  /* Test block erase */
  if (!memoryTestSequence(flash, IF_FLASH_BLOCK_SIZE, IF_FLASH_ERASE_BLOCK))
    pinWrite(led, !BOARD_LED_INV);

  while (1);
  return 0;
}
