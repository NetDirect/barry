///
/// \file	usbwrap_libusb.h
///		USB API wrapper for libusb version 0.1
///

/*
    Copyright (C) 2005-2011, Chris Frey
    Portions Copyright (C) 2011, RealVNC Ltd.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License in the COPYING file at the
    root directory of this project for more details.
*/


#ifndef __SB_USBWRAP_LIBUSB_H__
#define __SB_USBWRAP_LIBUSB_H__

#include "usbwrap.h"
#include <string>

#include <usb.h>

#if defined( WIN32 )
// On Windows systems, usb.h includes <windows.h> which defines min/max,
// which causes trouble for other headers
#undef min
#undef max
#endif

namespace Usb
{

struct DeviceID
{
	struct usb_device* m_dev;
};

struct DeviceHandle
{
	struct usb_dev_handle* m_handle;
};

struct DeviceListImpl
{
	std::vector<DeviceID> m_devices;
};

struct EndpointDescriptorImpl
{
	struct usb_endpoint_descriptor m_desc;
};

struct InterfaceDescriptorImpl
{
	struct usb_interface_descriptor m_desc;
};

struct ConfigDescriptorImpl
{
	struct usb_config_descriptor m_desc;
};

struct DeviceDescriptorImpl
{
	struct usb_device* m_dev;
	struct usb_device_descriptor m_desc;
};

}; // namespace Usb

#endif // __SB_USBWRAP_LIBUSB_H__
