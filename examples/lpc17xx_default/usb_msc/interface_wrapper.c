/*
 * lpc17xx_default/usb_msc/interface_wrapper.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "interface_wrapper.h"
#include <assert.h>
/*----------------------------------------------------------------------------*/
static enum Result interfaceInit(void *, const void *);
static void interfaceDeinit(void *);
static void interfaceSetCallback(void *, void (*)(void *), void *);
static enum Result interfaceGetParam(void *, int, void *);
static enum Result interfaceSetParam(void *, int, const void *);
static size_t interfaceRead(void *, void *, size_t);
static size_t interfaceWrite(void *, const void *, size_t);
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const InterfaceWrapper =
    &(const struct InterfaceClass){
    .size = sizeof(struct InterfaceWrapper),
    .init = interfaceInit,
    .deinit = interfaceDeinit,

    .setCallback = interfaceSetCallback,
    .getParam = interfaceGetParam,
    .setParam = interfaceSetParam,
    .read = interfaceRead,
    .write = interfaceWrite
};
/*----------------------------------------------------------------------------*/
static enum Result interfaceInit(void *object, const void *configBase)
{
  const struct InterfaceWrapperConfig * const config = configBase;
  assert(config);

  struct InterfaceWrapper * const interface = object;

  interface->inversion = config->inversion;
  interface->pipe = config->pipe;

  interface->rx = pinInit(config->rx);
  assert(pinValid(interface->rx));
  pinOutput(interface->rx, false);
  interface->tx = pinInit(config->tx);
  assert(pinValid(interface->tx));
  pinOutput(interface->tx, false);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void interfaceDeinit(void *object __attribute__((unused)))
{
}
/*----------------------------------------------------------------------------*/
static void interfaceSetCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct InterfaceWrapper * const interface = object;
  ifSetCallback(interface->pipe, callback, argument);
}
/*----------------------------------------------------------------------------*/
static enum Result interfaceGetParam(void *object, int parameter, void *data)
{
  struct InterfaceWrapper * const interface = object;
  return ifGetParam(interface->pipe, parameter, data);
}
/*----------------------------------------------------------------------------*/
static enum Result interfaceSetParam(void *object, int parameter,
    const void *data)
{
  struct InterfaceWrapper * const interface = object;

  switch ((enum IfParameter)parameter)
  {
    case IF_RELEASE:
      pinWrite(interface->rx, interface->inversion);
      pinWrite(interface->tx, interface->inversion);
      break;

    default:
      break;
  }

  return ifSetParam(interface->pipe, parameter, data);
}
/*----------------------------------------------------------------------------*/
static size_t interfaceRead(void *object, void *buffer, size_t length)
{
  struct InterfaceWrapper * const interface = object;

  pinWrite(interface->rx, interface->inversion);
  return ifRead(interface->pipe, buffer, length);
}
/*----------------------------------------------------------------------------*/
static size_t interfaceWrite(void *object, const void *buffer, size_t length)
{
  struct InterfaceWrapper * const interface = object;

  pinWrite(interface->tx, interface->inversion);
  return ifWrite(interface->pipe, buffer, length);
}
