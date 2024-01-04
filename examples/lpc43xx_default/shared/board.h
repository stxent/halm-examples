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
#define BOARD_CAP_TIMER   PIN(PORT_6, 2)
#define BOARD_CAPTURE     BOARD_CAP_0
#define BOARD_PHY_RESET   PIN(PORT_5, 2)
#define BOARD_LED_0       PIN(PORT_5, 7)
#define BOARD_LED_1       PIN(PORT_5, 5)
#define BOARD_LED_2       PIN(PORT_4, 0)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
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

#define BOARD_USB_IND0    BOARD_USB0_IND0
#define BOARD_USB_IND1    BOARD_USB0_IND1
#define BOARD_USB_CDC_INT 0x81
#define BOARD_USB_CDC_RX  0x02
#define BOARD_USB_CDC_TX  0x83
#define BOARD_USB_MSC_RX  0x01
#define BOARD_USB_MSC_TX  0x81
/*----------------------------------------------------------------------------*/
struct Capture;
struct ClockClass;
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
void boardSetAdcTimerRate(struct Timer *, size_t, uint32_t);
void boardResetClock(void);
void boardSetupClockExt(void);
const struct ClockClass *boardSetupClockOutput(uint32_t divisor);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcDma(void);
struct Interface *boardSetupAdcOneShot(void);
struct StreamPackage boardSetupAdcStream(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupBod(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupCan(struct Timer *);
struct CapturePackage boardSetupCapture(void);
struct Interface *boardSetupDac(void);
struct StreamPackage boardSetupDacDma(void);
struct Interface *boardSetupEeprom(void);
struct Interface *boardSetupFlash(void);
struct Interface *boardSetupI2C(void);
struct Interface *boardSetupI2C0(void);
struct Interface *boardSetupI2C1(void);
struct Interface *boardSetupI2CSlave(void);
struct Interface *boardSetupI2CSlave0(void);
struct Interface *boardSetupI2CSlave1(void);
struct StreamPackage boardSetupI2S(void);
struct RtClock *boardSetupRtc(bool);
struct Interface *boardSetupSdio(bool, struct Timer *);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpi0(void);
struct Interface *boardSetupSpi1(void);
struct Interface *boardSetupSpiDma(void);
struct Interface *boardSetupSpiDma0(void);
struct Interface *boardSetupSpiDma1(void);
struct Interface *boardSetupSpim(struct Timer *);
struct Timer *boardSetupTimer(void);
struct Timer *boardSetupTimer0(void);
struct Timer *boardSetupTimer3(void);
struct Timer *boardSetupTimerAlarm(void);
struct Timer *boardSetupTimerRIT(void);
struct Timer *boardSetupTimerSCT(void);
struct Entity *boardSetupUsb(void);
struct Entity *boardSetupUsb0(void);
struct Entity *boardSetupUsb1(void);
struct Watchdog *boardSetupWdt(bool);
struct Watchdog *boardSetupWwdt(void);

/* Counter timer alias */
struct Timer *boardSetupCounterTimer(void);
/* General Purpose Timer */
struct Timer *boardSetupCounterTimerGPT(void);
/* State Configurable Timer alias */
struct Timer *boardSetupCounterTimerSCT(void);
/* Low part of the State Configurable Timer */
struct Timer *boardSetupCounterTimerSCTDivided(void);
/* Unified State Configurable Timer */
struct Timer *boardSetupCounterTimerSCTUnified(void);

/* PWM alias */
struct PwmPackage boardSetupPwm(bool);
/* State Configurable Timer alias */
struct PwmPackage boardSetupPwmSCT(bool);
/* High part of the State Configurable Timer */
struct PwmPackage boardSetupPwmSCTDivided(bool);
/* Unified State Configurable Timer */
struct PwmPackage boardSetupPwmSCTUnified(bool);
/*----------------------------------------------------------------------------*/
#endif /* LPC43XX_DEFAULT_SHARED_BOARD_H_ */
