/*
 * lpc43xx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC43XX_DEFAULT_SHARED_BOARD_H_
#define LPC43XX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON      PIN(PORT_2, 7)
#define BOARD_CAP_0       PIN(7, 3)
#define BOARD_CAP_1       PIN(2, 13)
#define BOARD_CAPTURE     BOARD_CAP_0
#define BOARD_PHY_RESET   PIN(PORT_5, 2)
#define BOARD_LED_0       PIN(PORT_5, 7)
#define BOARD_LED_1       PIN(PORT_5, 5)
#define BOARD_LED_2       PIN(PORT_4, 0)
#define BOARD_LED         BOARD_LED_0
#define BOARD_PWM_0       PIN(PORT_4, 1)
#define BOARD_PWM_1       PIN(PORT_7, 7)
#define BOARD_PWM_2       PIN(PORT_7, 6)
#define BOARD_PWM_3       PIN(PORT_7, 5)
#define BOARD_PWM_4       PIN(PORT_7, 4)
#define BOARD_PWM_5       PIN(PORT_7, 0)
#define BOARD_PWM         BOARD_PWM_0
#define BOARD_SPI0_CS0    PIN(PORT_1, 0)
#define BOARD_SPI0_CS1    PIN(PORT_5, 0)
#define BOARD_SPI1_CS0    PIN(PORT_1, 5)
#define BOARD_SPI_CS      PIN(PORT_5, 0)
#define BOARD_USB0_IND0   PIN(PORT_6, 8)
#define BOARD_USB0_IND1   PIN(PORT_6, 7)
#define BOARD_UART_BUFFER 512
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
void boardSetupClockInt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcDma(void);
struct Interface *boardSetupAdcOneShot(void);
struct StreamPackage boardSetupAdcStream(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupCan(struct Timer *);
struct CapturePackage boardSetupCapture(void);
struct Interface *boardSetupDac(void);
struct StreamPackage boardSetupDacDma(void);
struct Interface *boardSetupI2C(void);
struct Interface *boardSetupI2C0(void);
struct Interface *boardSetupI2C1(void);
struct Interface *boardSetupI2CSlave(void);
struct Interface *boardSetupI2CSlave0(void);
struct Interface *boardSetupI2CSlave1(void);
struct StreamPackage boardSetupI2S(void);
struct PwmPackage boardSetupPwm(bool);
struct Timer *boardSetupRit(void);
struct RtClock *boardSetupRtc(void);
struct Interface *boardSetupSdmmc(bool);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpi0(void);
struct Interface *boardSetupSpi1(void);
struct Interface *boardSetupSpiDma(void);
struct Interface *boardSetupSpiDma0(void);
struct Interface *boardSetupSpiDma1(void);
struct Interface *boardSetupSpifi(void);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb0(void);
struct Entity *boardSetupUsb1(void);
struct Watchdog *boardSetupWdt(void);
/*----------------------------------------------------------------------------*/
#endif /* LPC43XX_DEFAULT_SHARED_BOARD_H_ */
