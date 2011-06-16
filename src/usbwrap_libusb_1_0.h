///
/// \file	usbwrap_libusb_1_0.h
///		USB API wrapper for libusb version 1.0
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

#ifndef __SB_USBWRAP_LIBUSB_1_0_H__
#define __SB_USBWRAP_LIBUSB_1_0_H__

#include "usbwrap.h"
#include <libusb.h>

#if defined( WIN32 )
// On Windows systems, libusb.h includes <windows.h> which defines min/max,
// which causes trouble for other headers
#undef min
#undef max
#endif

namespace Usb
{

struct DeviceID
{
	libusb_device *m_dev;
	std::string m_busname;
	std::string m_filename;
};

struct DeviceHandle
{
	libusb_device_handle *m_handle;
};

struct DeviceListImpl
{
	libusb_device** m_list;
	ssize_t m_listcnt;
	std::vector<DeviceID> m_devices;
};

struct EndpointDescriptorImpl
{
	const struct libusb_endpoint_descriptor* m_desc;
};

struct InterfaceDescriptorImpl
{
	const struct libusb_interface_descriptor* m_desc;
};

struct ConfigDescriptorImpl
{
	struct libusb_config_descriptor* m_desc;
};

struct DeviceDescriptorImpl
{
	struct libusb_device_descriptor m_desc;
	struct DeviceID* m_devid;
};

}; // namespace Usb

#endif // __SB_USBWRAP_LIBUSB_1_0_H__
