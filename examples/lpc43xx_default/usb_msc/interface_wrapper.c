/*
 * interface_wrapper.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include "interface_wrapper.h"
/*----------------------------------------------------------------------------*/
static enum result interfaceInit(void *, const void *);
static void interfaceDeinit(void *);
static enum result interfaceSetCallback(void *, void (*)(void *), void *);
static enum result interfaceGetParam(void *, enum IfParameter, void *);
static enum result interfaceSetParam(void *, enum IfParameter, const void *);
static size_t interfaceRead(void *, void *, size_t);
static size_t interfaceWrite(void *, const void *, size_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass interfaceTable = {
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
const struct InterfaceClass * const InterfaceWrapper = &interfaceTable;
/*----------------------------------------------------------------------------*/
static enum result interfaceInit(void *object, const void *configBase)
{
  const struct InterfaceWrapperConfig * const config = configBase;
  struct InterfaceWrapper * const interface = object;

  interface->pipe = config->pipe;
  interface->rx = pinInit(config->rx);
  assert(pinValid(interface->rx));
  pinOutput(interface->rx, 0);
  interface->tx = pinInit(config->tx);
  assert(pinValid(interface->tx));
  pinOutput(interface->tx, 0);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void interfaceDeinit(void *object __attribute__((unused)))
{

}
/*----------------------------------------------------------------------------*/
static enum result interfaceSetCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct InterfaceWrapper * const interface = object;
  return ifSetCallback(interface->pipe, callback, argument);
}
/*----------------------------------------------------------------------------*/
static enum result interfaceGetParam(void *object, enum IfParameter parameter,
    void *data)
{
  struct InterfaceWrapper * const interface = object;
  return ifGetParam(interface->pipe, parameter, data);
}
/*----------------------------------------------------------------------------*/
static enum result interfaceSetParam(void *object, enum IfParameter parameter,
    const void *data)
{
  struct InterfaceWrapper * const interface = object;

  switch (parameter)
  {
    case IF_RELEASE:
      pinReset(interface->rx);
      pinReset(interface->tx);
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

  pinSet(interface->rx);
  return ifRead(interface->pipe, buffer, length);
}
/*----------------------------------------------------------------------------*/
static size_t interfaceWrite(void *object, const void *buffer, size_t length)
{
  struct InterfaceWrapper * const interface = object;

  pinSet(interface->tx);
  return ifWrite(interface->pipe, buffer, length);
}
