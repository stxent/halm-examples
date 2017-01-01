/*
 * interface_wrapper.h
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef INTERFACE_WRAPPER_H_
#define INTERFACE_WRAPPER_H_
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
  pinNumber rx;
  /** Mandatory: transmission indication. */
  pinNumber tx;
};
/*----------------------------------------------------------------------------*/
struct InterfaceWrapper
{
  struct Interface base;

  struct Interface *pipe;
  struct Pin rx, tx;
};
/*----------------------------------------------------------------------------*/
#endif /* INTERFACE_WRAPPER_H_ */
