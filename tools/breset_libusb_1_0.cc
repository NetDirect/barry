///
/// \file	breset_libusb_1_0.cc
///		Attempt to reset all connected Blackberry devices via software
///
///		This file is part of the Barry project:
///
///		http://www.netdirect.ca/software/packages/barry
///		http://sourceforge.net/projects/barry
///
///		Compile with the following command (needs libusb 1.0):
///
///		g++ -o breset breset.cc -I/usr/include/libusb -lusb-1.0
///

/*
    Copyright (C) 2007-2011, Net Direct Inc. (http://www.netdirect.ca/)
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

#include <libusb.h>
#include <stdio.h>
#include <unistd.h>
#include "i18n.h"

#define VENDOR_RIM		0x0fca
#define PRODUCT_RIM_BLACKBERRY	0x0001
#define PRODUCT_RIM_PEARL_DUAL	0x0004
#define PRODUCT_RIM_PEARL_8120	0x8004
#define PRODUCT_RIM_PEARL	0x0006

#define BLACKBERRY_INTERFACE		0
#define BLACKBERRY_CONFIGURATION	1

#define BUSNAME_FORMAT_STR "libusb-%d"
// USB bus numbers can only go up to 256, i.e. 3 characters
#define MAX_BUSNAME_LENGTH (7 + 3 + 1)

bool reset(struct libusb_device *dev)
{
	libusb_device_handle *handle = NULL;
	int err = libusb_open(dev, &handle);
	if( err != 0 )
		return false;

	bool ret = libusb_reset_device(handle) == 0;
	libusb_close(handle);
	return ret;
}

int main()
{
	libusb_context *usbctx = NULL;
	libusb_device **devices = NULL;
	int found = 0;
	int count = 0;
	int i;

	INIT_I18N(PACKAGE);

	int err = libusb_init(&usbctx);
	if( err != 0 ) {
		printf(_("Failed to start USB: %d\n"), err);
	}

	printf(_("Scanning for Blackberry devices...\n"));

	count = libusb_get_device_list(usbctx, &devices);

	for(i = 0; i < count; ++i ) {
		libusb_device *dev = devices[i];
		uint8_t devnum = libusb_get_device_address(dev);
		uint8_t busnum = libusb_get_bus_number(dev);
	        struct libusb_device_descriptor desc;
		char busname[MAX_BUSNAME_LENGTH];
		int err = libusb_get_device_descriptor(dev, &desc);
		snprintf(busname, sizeof(busname),
			 BUSNAME_FORMAT_STR, busnum);
		// Be sure it's nul terminated
		busname[MAX_BUSNAME_LENGTH - 1] = 0;
		
		// Is this a blackberry?
		if( err == 0 &&
		    desc.idVendor == VENDOR_RIM &&
		    (desc.idProduct == PRODUCT_RIM_BLACKBERRY ||
		     desc.idProduct == PRODUCT_RIM_PEARL ||
		     desc.idProduct == PRODUCT_RIM_PEARL_8120 ||
		     desc.idProduct == PRODUCT_RIM_PEARL_DUAL ) ) {
			printf(_("Found..."));
			printf(_("attempting to reset.\n"));
			if( reset(dev) )
				found++;
			else
				printf(_("Can't reset device on bus %s, devnum %u\n"), busname, devnum);
		}
	}

	printf(_("%d device%s reset.\n"), found, found > 1 ? "s" : "");
	
	if( devices )
		libusb_free_device_list(devices, 1);
	libusb_exit(usbctx);
}

