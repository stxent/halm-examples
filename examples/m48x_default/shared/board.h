/*
 * m48x_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef M48X_DEFAULT_SHARED_BOARD_H_
#define M48X_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON_0    PIN(PORT_G, 15)
#define BOARD_BUTTON_1    PIN(PORT_F, 11)
#define BOARD_BUTTON      BOARD_BUTTON_0
#define BOARD_LED_0       PIN(PORT_H, 0)
#define BOARD_LED_1       PIN(PORT_H, 1)
#define BOARD_LED_2       PIN(PORT_H, 2)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     true
#define BOARD_PWM_0       PIN(PORT_A, 3)
#define BOARD_PWM_1       PIN(PORT_A, 4)
#define BOARD_PWM_2       PIN(PORT_A, 5)
#define BOARD_PWM         BOARD_PWM_0
#define BOARD_SPI_CS      PIN(PORT_C, 9)
#define BOARD_QSPI_CS     PIN(PORT_C, 3)
#define BOARD_UART_BUFFER 512
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Interrupt;
struct Pwm;
struct Timer;
struct Watchdog;

struct PwmPackage
{
  struct Timer *timer;
  struct Pwm *output;
  struct Pwm *outputs[3];
};
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void);
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcDma(void);
struct Timer *boardSetupAdcTimer(void);
struct PwmPackage boardSetupBpwm(bool);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupCan(struct Timer *);
struct Entity *boardSetupHsUsb(void);
struct Interface *boardSetupI2C(void);
struct Interface *boardSetupQspi(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSerialDmaTOC(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpiDma(void);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb(void);
struct Watchdog *boardSetupWdt(bool);
/*----------------------------------------------------------------------------*/
#endif /* M48X_DEFAULT_SHARED_BOARD_H_ */
