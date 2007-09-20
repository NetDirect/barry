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
    Copyright (C) 2006-2007, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>

#define VENDOR_RIM		0x0fca
#define PRODUCT_RIM_BLACKBERRY	0x0001
#define PRODUCT_RIM_PEARL_DUAL	0x0004
#define PRODUCT_RIM_PEARL	0x0006

#define IPRODUCT_RIM_HANDHELD		2
#define IPRODUCT_RIM_MASS_STORAGE	4
#define IPRODUCT_RIM_COMPOSITE		5

#define BLACKBERRY_INTERFACE		0
#define BLACKBERRY_CONFIGURATION	1

bool old_style_pearl = false;
bool force_dual = false;
std::string udev_devpath;
std::string sysfs_path = "/sys";

void Usage()
{
	printf(
	"bcharge - Adjust Blackberry charging modes\n"
	"          Copyright 2006-2007, Net Direct Inc. (http://www.netdirect.ca/)\n"
	"\n"
	"   -d          Dual mode (mode 0004) (default)\n"
	"   -o          Set a Pearl to old Blackberry mode (0001)\n"
	"\n"
	"   -h          This help message\n"
	"   -p devpath  The devpath argument from udev.  If specified, will attempt\n"
	"               to adjust USB suspend settings to keep the device charging.\n"
	"   -s path     The path where sysfs is mounted.  Defaults to '/sys'\n"
	"\n"
	);
}

void control(usb_dev_handle *dev, int requesttype, int request, int value,
	int index, char *bytes, int size, int timeout)
{
	int result = usb_control_msg(dev, requesttype, request, value, index,
		bytes, size, timeout);
	if( result < 0 ) {
		printf("\nusb_control_msg failed: code: %d, %s\n", result,
			usb_strerror());
	}
}

void charge(struct usb_dev_handle *handle)
{
	// the special sauce... these steps seem to do the trick
	// for the 7750 series... needs testing on others
	char buffer[2];
	control(handle, 0xc0, 0xa5, 0, 1, buffer, 2, 100);
	control(handle, 0x40, 0xa2, 0, 1, buffer, 0, 100);
}

void pearl_mode(struct usb_dev_handle *handle)
{
	char buffer[2];
	if( old_style_pearl ) {
		// use this for "old style" interface: product ID 0001
		control(handle, 0xc0, 0xa9, 0, 1, buffer, 2, 100);
	}
	else {
		// Product ID 0004
		control(handle, 0xc0, 0xa9, 1, 1, buffer, 2, 100);
	}
}

// returns true if device mode was modified, false otherwise
bool process(struct usb_device *dev, bool is_pearl)
{
	bool apply = false;
	printf("Found device #%s...", dev->filename);

	// open
	usb_dev_handle *handle = usb_open(dev);
	if( !handle ) {
		printf("unable to open device\n");
		return false;
	}

	// adjust power
	if( dev->config &&
	    dev->descriptor.bNumConfigurations >= 1 &&
	    dev->config[0].MaxPower < 250 ) {
		printf("adjusting charge setting");
		charge(handle);
		apply = true;
	}
	else {
		printf("already at 500mA");
	}

	// adjust Pearl mode
	if( is_pearl || force_dual ) {
		int desired_mode = old_style_pearl
			? PRODUCT_RIM_BLACKBERRY : PRODUCT_RIM_PEARL_DUAL;

		if( desired_mode != dev->descriptor.idProduct ) {
			printf("...adjusting Pearl mode to %s",
				old_style_pearl ? "single" : "dual");
			pearl_mode(handle);
			apply = true;
		}
		else {
			printf("...already in desired Pearl mode");
		}
	}
	else {
		printf("...no Pearl adjustment");
	}

	// apply changes
	if( apply ) {
		// usb_set_configuration may fail here, and that's ok...
		// likely means the device is in the process of resetting
		usb_set_configuration(handle, BLACKBERRY_CONFIGURATION);

		// the Blackberry Pearl doesn't reset itself after the above,
		// so do it ourselves
		if( is_pearl || force_dual ) {
			if( usb_reset(handle) < 0 ) {
				printf("\nusb_reset failed: %s\n", usb_strerror());
			}
		}

		printf("...done\n");
	}
	else {
		printf("...no change\n");
	}

	// cleanup
	usb_close(handle);
	return apply;
}

// checks for USB suspend, and enables the device if suspended
void resume()
{
	if( udev_devpath.size() == 0 )
		return;	// nothing to do

	// let sysfs settle a bit
	sleep(5);

	// attempt to open the state file
	std::string status = sysfs_path + "/" + udev_devpath + "/device/power/state";
	int fd = open(status.c_str(), O_RDWR);
	if( fd == -1 ) {
		printf("unable to adjust USB suspend/resume settings (using sysfs file: %s): %s\n",
			status.c_str(),
			strerror(errno));
		return;
	}

	write(fd, "0", 1);
	close(fd);
}

int main(int argc, char *argv[])
{
	struct usb_bus *busses;

	//
	// allow -o command line switch to choose which mode to use for
	// Blackberry Pearls:
	//	Dual(default):  0004	-d
	//	With switch:    0001	-o
	//

	// process command line options
	for(;;) {
		int cmd = getopt(argc, argv, "dop:s:h");
		if( cmd == -1 )
			break;

		switch( cmd )
		{
		case 'd':	// Dual (default)
			force_dual = true;
			old_style_pearl = false;
			break;

		case 'o':	// Old style pearl
			force_dual = false;
			old_style_pearl = true;
			break;

		case 'p':	// udev devpath
			udev_devpath = optarg;
			break;

		case 's':	// where sysfs is mounted
			sysfs_path = optarg;
			break;

		case 'h':	// help!
		default:
			Usage();
			return 0;
		}
	}

	usb_init();
	if( usb_find_busses() < 0 || usb_find_devices() < 0 ) {
		printf("\nUnable to scan devices: %s\n", usb_strerror());
		return 1;
	}
	busses = usb_get_busses();

	printf("Scanning for Blackberry devices...\n");

	struct usb_bus *bus;
	for( bus = busses; bus; bus = bus->next ) {
		struct usb_device *dev;

		for (dev = bus->devices; dev; dev = dev->next) {
			// Is this a blackberry?
			if( dev->descriptor.idVendor == VENDOR_RIM ) {
				switch(dev->descriptor.idProduct)
				{
				case PRODUCT_RIM_BLACKBERRY:
					if( !process(dev, false) )
						resume();
					break;

				case PRODUCT_RIM_PEARL_DUAL:
				case PRODUCT_RIM_PEARL:
					if( !process(dev, true) )
						resume();
					break;
				}
			}
		}
	}
}

