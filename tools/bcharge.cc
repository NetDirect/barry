///
/// \file	bcharge.cc
///		Talk to the Blackberry just enough to change the Max Power
///		to 500mA.  Cycles through all devices attached to USB,
///		attempting to set all matching Blackberry devices to charge.
///
///		This file is part of the Barry project:
///
///		http://www.netdirect.ca/software/packages/barry/index.php
///		http://sourceforge.net/projects/barry
///
///		Compile with the following command (needs libusb):
///
///		g++ -o bcharge bcharge.cc -lusb
///

/*
    Copyright (C) 2006, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <usb.h>
#include <stdio.h>
#include <unistd.h>

#define VENDOR_RIM		0x0fca
#define PRODUCT_RIM_BLACKBERRY	0x0001

#define BLACKBERRY_INTERFACE		0
#define BLACKBERRY_CONFIGURATION	1

void charge(struct usb_device *dev)
{
	usb_dev_handle *handle = usb_open(dev);
	if( !handle )
		return;

	// the special sauce... these 3 steps seem to do the trick
	// for the 7750 series... needs testing on others
	char buffer[2];
	usb_control_msg(handle, 0xc0, 0xa5, 0, 1, buffer, 2, 100);
	usb_control_msg(handle, 0x40, 0xa2, 0, 1, buffer, 0, 100);
	usb_set_configuration(handle, BLACKBERRY_CONFIGURATION);

	// let it reset itself
	sleep(3);

	// cleanup
	usb_close(handle);
}

int main()
{
	struct usb_bus *busses;

	usb_init();
	usb_find_busses();
	usb_find_devices();
	busses = usb_get_busses();

	printf("Scanning for Blackberry devices...\n");
	int found = 0;

	struct usb_bus *bus;
	for( bus = busses; bus; bus = bus->next ) {
		struct usb_device *dev;

		for (dev = bus->devices; dev; dev = dev->next) {
			// Is this a blackberry?
			if( dev->descriptor.idVendor == VENDOR_RIM &&
			    dev->descriptor.idProduct == PRODUCT_RIM_BLACKBERRY ) {
			    	printf("Found...");
				if( dev->config &&
				    dev->descriptor.bNumConfigurations >= 1 &&
				    dev->config[0].MaxPower < 250 ) {
					printf("attempting to adjust charge setting.\n");
					charge(dev);
					found++;
				}
				else {
					printf("already at 500mA\n");
				}
			}
		}
	}

	printf("%d device%s adjusted.\n", found, found > 1 ? "s" : "");
}

