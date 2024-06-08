/*
 * lpc43xx_default/eth_raw/main.c
 * Copyright (C) 2021 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <xcore/interface.h>
#include <xcore/memory.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define PHY_REG_BMCR                0x00
#define PHY_REG_BMSR                0x01
#define PHY_REG_IDR1                0x02
#define PHY_REG_IDR2                0x03

#define PHY_BMCR_AUTO_NEG_RESTART   0x0200
#define PHY_BMCR_AUTO_NEG_ENABLE    0x1000
#define PHY_BMCR_RESET              0x8000
#define PHY_BMSR_LINK_UP            0x0004
#define PHY_BMSR_AUTO_NEG_COMPLETED 0x0020
/*----------------------------------------------------------------------------*/
#define ETHER_TYPE  0x9000
#define OPCODE_BUF  0x0001
#define OPCODE_REQ  0x0002

#define ARENA_SIZE  16384
#define CHUNK_SIZE  1024

/* 1e:30:6c:a2:45:5e */
#define MAC_ADDRESS 0x5E45A26C301EUL
/*----------------------------------------------------------------------------*/
struct [[gnu::packed]] EthHeader
{
  /* Ethernet address of destination */
  uint8_t dst[6];
  /* Ethernet address of sender */
  uint8_t src[6];
  /* Protocol type */
  uint16_t type;
};

struct [[gnu::packed]] TestPacket
{
  struct EthHeader header;
  /* Operation code */
  uint16_t opcode;
  /* Offset from the beginning of the buffer */
  uint32_t offset;
  /* Payload length */
  uint32_t length;
  /* Payload */
  uint8_t data[];
};
/*----------------------------------------------------------------------------*/
static void handleEthFrame(struct Interface *, const uint8_t *, size_t);
static bool phyInit(struct Interface *);
static uint16_t phyReadReg(struct Interface *, uint32_t);
static void phyWriteReg(struct Interface *, uint32_t, uint16_t);
static void sendMemChunk(struct Interface *, const uint8_t *, size_t, size_t);
/*----------------------------------------------------------------------------*/
static uint8_t arena[ARENA_SIZE];
/*----------------------------------------------------------------------------*/
static void onFrameReady(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void handleEthFrame(struct Interface *eth, const uint8_t *frame,
    size_t length)
{
  const struct EthHeader * const header = (const struct EthHeader *)frame;

  if (fromBigEndian16(header->type) == ETHER_TYPE)
  {
    const struct TestPacket * const packet = (const struct TestPacket *)frame;

    if (length < sizeof(struct TestPacket))
      return;

    switch (fromBigEndian16(packet->opcode))
    {
      case OPCODE_BUF:
      {
        const size_t chunkOffset = fromBigEndian32(packet->offset);
        const size_t chunkLength = fromBigEndian32(packet->length);

        if (length < sizeof(struct TestPacket) + chunkLength)
          break;
        if (chunkOffset + chunkLength > ARENA_SIZE)
          break;

        memcpy(arena + chunkOffset, packet->data, chunkLength);
        break;
      }

      case OPCODE_REQ:
      {
        for (size_t offset = 0; offset < ARENA_SIZE; offset += CHUNK_SIZE)
          sendMemChunk(eth, packet->header.src, offset, CHUNK_SIZE);
        break;
      }

      default:
        break;
    }
  }
}
/*----------------------------------------------------------------------------*/
static bool phyInit(struct Interface *mdio)
{
  static const uint32_t PHY_ADDRESS = 0x00;
  static const uint32_t PHY_TIMEOUT = 1000;

  ifSetParam(mdio, IF_ADDRESS, &PHY_ADDRESS);

  /* Put the PHY in reset mode */
  phyWriteReg(mdio, PHY_REG_BMCR, PHY_BMCR_RESET);

  /* Wait for hardware reset to end */
  for (uint32_t timeout = 0; timeout < PHY_TIMEOUT; ++timeout)
  {
    const uint16_t value = phyReadReg(mdio, PHY_REG_BMCR);

    if (!(value & PHY_BMCR_RESET))
      break;

    mdelay(10);
  }

  /* Check if this is a LAN8720C PHY */
  const uint16_t idr1 = phyReadReg(mdio, PHY_REG_IDR1);
  const uint16_t idr2 = phyReadReg(mdio, PHY_REG_IDR2);

  if (idr1 == 0x0007 && idr2 == 0xC0F1)
  {
    bool autoNegotiationEnded = false;

    /* Enable and restart auto-negotiation */
    phyWriteReg(mdio, PHY_REG_BMCR,
        PHY_BMCR_AUTO_NEG_ENABLE | PHY_BMCR_AUTO_NEG_RESTART);

    /* Wait for auto-negotiation to end */
    for (uint32_t timeout = 0; timeout < PHY_TIMEOUT; ++timeout)
    {
      const uint16_t value = phyReadReg(mdio, PHY_REG_BMSR);

      if (value & PHY_BMSR_AUTO_NEG_COMPLETED)
      {
        autoNegotiationEnded = true;
        break;
      }

      mdelay(10);
    }

    if (!autoNegotiationEnded)
      return false;

    /* Check the link status */
    for (uint32_t timeout = 0; timeout < PHY_TIMEOUT; ++timeout)
    {
      const uint16_t value = phyReadReg(mdio, PHY_REG_BMSR);

      if (value & PHY_BMSR_LINK_UP)
        return true;

      mdelay(10);
    }

    return false;
  }

  return false;
}
/*----------------------------------------------------------------------------*/
static uint16_t phyReadReg(struct Interface *mdio, uint32_t address)
{
  uint16_t value;

  ifSetParam(mdio, IF_POSITION, &address);
  ifRead(mdio, &value, sizeof(value));

  return value;
}
/*----------------------------------------------------------------------------*/
static void phyWriteReg(struct Interface *mdio, uint32_t address,
    uint16_t value)
{
  ifSetParam(mdio, IF_POSITION, &address);
  ifWrite(mdio, &value, sizeof(value));
}
/*----------------------------------------------------------------------------*/
static void sendMemChunk(struct Interface *eth, const uint8_t *mac,
    size_t offset, size_t length)
{
  uint64_t address;
  uint8_t frame[BOARD_ETH_BUFFER];
  struct TestPacket * const packet = (struct TestPacket *)frame;

  ifGetParam(eth, IF_ADDRESS_64, &address);
  packet->header.src[0] = address >> 0;
  packet->header.src[1] = address >> 8;
  packet->header.src[2] = address >> 16;
  packet->header.src[3] = address >> 24;
  packet->header.src[4] = address >> 32;
  packet->header.src[5] = address >> 40;

  memcpy(packet->header.dst, mac, sizeof(packet->header.dst));
  packet->header.type = TO_BIG_ENDIAN_16(ETHER_TYPE);

  packet->opcode = TO_BIG_ENDIAN_16(OPCODE_BUF);
  packet->offset = toBigEndian32(offset);
  packet->length = length;
  memcpy(packet->data, arena + offset, length);

  while (ifWrite(eth, frame, sizeof(struct TestPacket) + length) == 0);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  const struct Pin rst = pinInit(BOARD_PHY_RESET);
  pinOutput(rst, true);

  struct EthernetPackage eth = boardSetupEthernet(MAC_ADDRESS, 4, 4);

  const bool phyReady = phyInit(eth.mdio);
  (void)phyReady;
  assert(phyReady);

  bool event = false;
  ifSetCallback(eth.proxy, onFrameReady, &event);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    uint8_t buffer[BOARD_ETH_BUFFER];
    size_t count;

    pinToggle(led);
    while ((count = ifRead(eth.proxy, buffer, sizeof(buffer))) > 0)
      handleEthFrame(eth.proxy, buffer, count);
    pinToggle(led);
  }

  return 0;
}
