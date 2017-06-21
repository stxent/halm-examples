/*
 * lpc43xx_default/usb_msc/interface_wrapper.h
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC43XX_DEFAULT_USB_MSC_INTERFACE_WRAPPER_H_
#define LPC43XX_DEFAULT_USB_MSC_INTERFACE_WRAPPER_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <xcore/interface.h>
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const InterfaceWrapper;
/*----------------------------------------------------------------------------*/
struct InterfaceWrapperConfig
{
  /** Mandatory: underlying interface. */
  struct Interface *pipe;
  /** Mandatory: reception indication. */
  PinNumber rx;
  /** Mandatory: transmission indication. */
  PinNumber tx;
};
/*----------------------------------------------------------------------------*/
struct InterfaceWrapper
{
  struct Interface base;

  struct Interface *pipe;
  struct Pin rx, tx;
};
/*----------------------------------------------------------------------------*/
#endif /* LPC43XX_DEFAULT_USB_MSC_INTERFACE_WRAPPER_H_ */
