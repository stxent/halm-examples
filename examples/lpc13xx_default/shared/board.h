/*
 * lpc13xx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC13XX_DEFAULT_SHARED_BOARD_H_
#define LPC13XX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON      PIN(0, 3)
#define BOARD_CAPTURE     PIN(1, 5)
#define BOARD_LED_0       PIN(2, 3)
#define BOARD_LED_1       PIN(3, 1)
#define BOARD_LED_2       PIN(3, 0)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_PWM_0       PIN(1, 10)
#define BOARD_PWM_1       PIN(1, 9)
#define BOARD_PWM         BOARD_PWM_0
#define BOARD_SPI_CS      PIN(0, 2)
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Capture;
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
  struct Pwm *outputs[2];
};
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void);
void boardSetupClockExt(void);
void boardSetupClockOutput(uint32_t);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcOneShot(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupButton(void);
struct CapturePackage boardSetupCapture(void);
struct Timer *boardSetupCounterTimer(void);
struct Interface *boardSetupI2C(void);
struct PwmPackage boardSetupPwm(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialPoll(void);
struct Interface *boardSetupSpi(void);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb(void);
struct Watchdog *boardSetupWdt(void);
/*----------------------------------------------------------------------------*/
#endif /* LPC13XX_DEFAULT_SHARED_BOARD_H_ */