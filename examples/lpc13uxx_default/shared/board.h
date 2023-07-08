/*
 * lpc13uxx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC13UXX_DEFAULT_SHARED_BOARD_H_
#define LPC13UXX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON      PIN(0, 3)
#define BOARD_CAPTURE     PIN(0, 17)
#define BOARD_LED_0       PIN(1, 22)
#define BOARD_LED_1       PIN(1, 14)
#define BOARD_LED_2       PIN(1, 13)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_PWM_0       PIN(0, 22)
#define BOARD_PWM_1       PIN(0, 21)
#define BOARD_PWM         BOARD_PWM_0
#define BOARD_SPI_CS      PIN(0, 2)
#define BOARD_UART_BUFFER 128

#define BOARD_USB_IND0    BOARD_LED_1
#define BOARD_USB_IND1    BOARD_LED_2
#define BOARD_USB_CDC_INT 0x81
#define BOARD_USB_CDC_RX  0x02
#define BOARD_USB_CDC_TX  0x82
/*----------------------------------------------------------------------------*/
struct Capture;
struct ClockClass;
struct Entity;
struct Interface;
struct Interrupt;
struct Pwm;
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
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void);
void boardSetAdcTimerRate(struct Timer *, size_t, uint32_t);
void boardSetupClockExt(void);
const struct ClockClass *boardSetupClockOutput(uint32_t);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcOneShot(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupBod(void);
struct Interrupt *boardSetupButton(void);
struct CapturePackage boardSetupCapture(void);
struct Timer *boardSetupCounterTimer(void);
struct Interface *boardSetupEeprom(void);
struct Interface *boardSetupFlash(void);
struct Interface *boardSetupI2C(void);
struct PwmPackage boardSetupPwm(bool);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSpi(void);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb(void);
struct Watchdog *boardSetupWdt(bool);
/*----------------------------------------------------------------------------*/
#endif /* LPC13UXX_DEFAULT_SHARED_BOARD_H_ */
