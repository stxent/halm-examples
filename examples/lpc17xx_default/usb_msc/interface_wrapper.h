/*
 * lpc17xx_default/usb_msc/interface_wrapper.h
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC17XX_DEFAULT_USB_MSC_INTERFACE_WRAPPER_H_
#define LPC17XX_DEFAULT_USB_MSC_INTERFACE_WRAPPER_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <xcore/interface.h>
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const InterfaceWrapper;

struct InterfaceWrapperConfig
{
  /** Mandatory: underlying interface. */
  struct Interface *pipe;
  /** Mandatory: reception indication. */
  PinNumber rx;
  /** Mandatory: transmission indication. */
  PinNumber tx;
  /** Optional: enable output inversion. */
  bool inversion;
};

struct InterfaceWrapper
{
  struct Interface base;

  struct Interface *pipe;
  struct Pin rx;
  struct Pin tx;
  bool inversion;
};
/*----------------------------------------------------------------------------*/
#endif /* LPC17XX_DEFAULT_USB_MSC_INTERFACE_WRAPPER_H_ */
