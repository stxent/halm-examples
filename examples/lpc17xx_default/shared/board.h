/*
 * lpc17xx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC17XX_DEFAULT_SHARED_BOARD_H_
#define LPC17XX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON      PIN(2, 10)
#define BOARD_CAPTURE     PIN(1, 18)
#define BOARD_LED_0       PIN(1, 10)
#define BOARD_LED_1       PIN(1, 9)
#define BOARD_LED_2       PIN(1, 8)
#define BOARD_LED         BOARD_LED_0
#define BOARD_PWM_0       PIN(1, 23)
#define BOARD_PWM_1       PIN(1, 24)
#define BOARD_PWM_2       PIN(1, 20)
#define BOARD_PWM         BOARD_PWM_0
#define BOARD_SPI0_CS0    PIN(0, 22)
#define BOARD_SPI1_CS0    PIN(0, 6)
#define BOARD_SPI1_CS1    PIN(1, 15)
#define BOARD_SDIO_CS     BOARD_SPI0_CS0
#define BOARD_SPI_CS      BOARD_SPI1_CS0
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Capture;
struct Entity;
struct Interface;
struct Interrupt;
struct Pwm;
struct RtClock;
struct Stream;
struct Timer;
struct Watchdog;

struct CapturePackage
{
  struct Timer *timer;
  struct Capture *input;
};

struct PwmPackage
{
  struct Timer *timer;
  struct Pwm *output;
  struct Pwm *outputs[3];
};

struct StreamPackage
{
  struct Interface *interface;
  struct Stream *rx;
  struct Stream *tx;
};
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void);
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcDma(void);
struct Interface *boardSetupAdcOneShot(void);
struct StreamPackage boardSetupAdcStream(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupCan(struct Timer *);
struct CapturePackage boardSetupCapture(void);
struct Timer *boardSetupCounterTimer(void);
struct Interface *boardSetupDac(void);
struct StreamPackage boardSetupDacDma(void);
struct Interface *boardSetupI2C(void);
struct Interface *boardSetupI2CSlave(void);
struct StreamPackage boardSetupI2S(void);
struct PwmPackage boardSetupPwm(void);
struct Timer *boardSetupRit(void);
struct RtClock *boardSetupRtc(void);
struct Interface *boardSetupSdioSpi(void);
struct Interface *boardSetupSdioSpiDma(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpi0(void);
struct Interface *boardSetupSpi1(void);
struct Interface *boardSetupSpiDma(void);
struct Interface *boardSetupSpiDma0(void);
struct Interface *boardSetupSpiDma1(void);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb(void);
struct Watchdog *boardSetupWdt(void);
/*----------------------------------------------------------------------------*/
#endif /* LPC17XX_DEFAULT_SHARED_BOARD_H_ */
