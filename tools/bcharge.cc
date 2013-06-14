///
/// \file	bcharge.cc
///		Talk to the Blackberry just enough to change the Max Power
///		to 500mA.  Cycles through all devices attached to USB,
///		attempting to set all matching Blackberry devices to charge.
///
///		This file is part of the Barry project:
///
///		http://www.netdirect.ca/software/packages/barry
///		http://sourceforge.net/projects/barry
///
///		Compile with the following command (needs libusb):
///
///		g++ -o bcharge bcharge.cc -lusb
///

/*
    Copyright (C) 2006-2013, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <barry/common.h>
#include "i18n.h"

enum ModeType {
	NO_CHANGE,
	PEARL_CLASSIC_MODE_0001,	// force classic mode
	PEARL_DUAL_MODE_0004,		// force dual mode
	CONDITIONAL_DUAL_MODE,		// set dual mode if no class 255
					// interface is found (database iface)
};

std::string udev_devpath;
std::string sysfs_path = "/sys";

void Usage()
{
	printf(_(
	"bcharge - Adjust Blackberry charging modes\n"
	"          Copyright 2006-2013, Net Direct Inc. (http://www.netdirect.ca/)\n"
	"\n"
	"   -d          Set to dual mode (0004)\n"
	"   -o          Set a Pearl to old Blackberry mode (0001)\n"
	"   -g          Set dual mode only if database interface class 255\n"
	"               is not found\n"
	"\n"
	"   -h          This help message\n"
	"   -p devpath  The devpath argument from udev.  If specified, will attempt\n"
	"               to adjust USB suspend settings to keep the device charging.\n"
	"   -s path     The path where sysfs is mounted.  Defaults to '/sys'\n"
	"\n")
	);
}

void control(usb_dev_handle *dev, int requesttype, int request, int value,
	int index, char *bytes, int size, int timeout)
{
	int result = usb_control_msg(dev, requesttype, request, value, index,
		bytes, size, timeout);
	if( result < 0 ) {
		printf(_("\nusb_control_msg failed: code: %d, %s\n"), result,
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

void pearl_classic_mode(struct usb_dev_handle *handle)
{
	char buffer[2];
	// use this for "old style" interface: product ID 0001
	control(handle, 0xc0, 0xa9, 0, 1, buffer, 2, 100);
}

void pearl_dual_mode(struct usb_dev_handle *handle)
{
	char buffer[2];
	// Product ID 0004
	control(handle, 0xc0, 0xa9, 1, 1, buffer, 2, 100);
}

int find_interface(struct usb_dev_handle *handle, int iface_class)
{
	// search the configuration descriptor for the given class ID
	struct usb_device *dev = usb_device(handle);
	struct usb_config_descriptor *cfg = dev ? dev->config : 0;

	if( cfg ) {

		for( unsigned i = 0; cfg->interface && i < cfg->bNumInterfaces; i++ ) {
			struct usb_interface *iface = &cfg->interface[i];
			for( int a = 0; iface->altsetting && a < iface->num_altsetting; a++ ) {
				struct usb_interface_descriptor *id = &iface->altsetting[a];
				if( id->bInterfaceClass == iface_class )
					return id->bInterfaceNumber;
			}
		}
	}

	return -1;
}

int find_mass_storage_interface(struct usb_dev_handle *handle)
{
	int iface = find_interface(handle, USB_CLASS_MASS_STORAGE);

	if( iface == -1 ) {
		// if we get here, then we didn't find the Mass Storage
		// interface ... this should never happen, but if it does,
		// assume the device is showing product ID 0006, and the
		// Mass Storage interface is interface #0
		printf(_("Can't find Mass Storage interface, assuming 0.\n"));
		return 0;
	}
	else {
		return iface;
	}
}

void driver_conflict(struct usb_dev_handle *handle)
{
	// this is called if the first usb_set_configuration()
	// failed... this most probably means that usb_storage
	// has already claimed the Mass Storage interface,
	// in which case we politely tell it to go away.

#if LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	printf("Detecting possible kernel driver conflict, trying to resolve...\n");

	int iface = find_mass_storage_interface(handle);
	if( usb_detach_kernel_driver_np(handle, iface) < 0 )
		printf(_("usb_detach_kernel_driver_np() failed: %s\n"), usb_strerror());

	if( usb_set_configuration(handle, BLACKBERRY_CONFIGURATION) < 0 )
		printf(_("usb_set_configuration() failed: %s\n"), usb_strerror());
#endif
}

// returns true if device mode was modified, false otherwise
bool process(struct usb_device *dev, ModeType mode)
{
	bool apply = false;
	printf(_("Found device #%s..."), dev->filename);

	// open
	usb_dev_handle *handle = usb_open(dev);
	if( !handle ) {
		printf(_("unable to open device\n"));
		return false;
	}

	// adjust power
	if( dev->config &&
	    dev->descriptor.bNumConfigurations >= 1 &&
	    dev->config[0].MaxPower < 250 ) {
		printf(_("adjusting charge setting"));
		charge(handle);
		apply = true;
	}
	else {
		printf(_("already at 500mA"));
	}
	printf("\n");

	// adjust Pearl mode
	switch( mode )
	{
	case NO_CHANGE:
		printf(_("...no Pearl mode adjustment"));
		break;

	case PEARL_CLASSIC_MODE_0001:
		if( dev->descriptor.idProduct != PRODUCT_RIM_BLACKBERRY ) {
			printf(_("...adjusting Pearl mode to single"));
			pearl_classic_mode(handle);
			apply = true;
		}
		else {
			printf(_("...already in classic/single mode"));
		}
		break;

	case PEARL_DUAL_MODE_0004:
		if( dev->descriptor.idProduct != PRODUCT_RIM_PEARL_DUAL ) {
			printf(_("...adjusting Pearl mode to dual"));
			pearl_dual_mode(handle);
			apply = true;
		}
		else {
			printf(_("...already in dual mode"));
		}
		break;

	case CONDITIONAL_DUAL_MODE:
		if( find_interface(handle, 255) == -1 ) {
			printf(_("...no database iface found, setting dual mode"));
			pearl_dual_mode(handle);
			apply = true;
		}
		else {
			printf(_("...found database iface, no change"));
		}
		break;

	default:
		printf("Bug: default case");
		break;
	}
	printf("\n");

	// apply changes
	if( apply ) {
		if( usb_set_configuration(handle, BLACKBERRY_CONFIGURATION) < 0 )
			driver_conflict(handle);

		// the Blackberry Pearl doesn't reset itself after the above,
		// so do it ourselves
		if( mode == PEARL_DUAL_MODE_0004 ) {
			//
			// It has been observed that the 8830 behaves both like
			// a Pearl device (in that it has mass storage +
			// database modes) as well as a Classic device in
			// that it resets itself and doesn't need an explicit
			// reset call.
			//
			// In order to deal with this, we insert a brief sleep.
			// If it is an 8830, it will reset itself and the
			// following reset call will fail.  If it is a Pearl,
			// the reset will work as normal.
			//
			sleep(1);
			if( usb_reset(handle) < 0 ) {
				printf(_("\nusb_reset failed: %s\n"), usb_strerror());
			}
		}

		printf(_("...done"));
	}
	else {
		printf(_("...no change"));
	}
	printf("\n");

	// cleanup
	usb_close(handle);
	return apply;
}

bool power_write(const std::string &file, const std::string &value)
{
	// attempt to open the state file
	int fd = open(file.c_str(), O_RDWR);
	if( fd == -1 ) {
		printf(_("autosuspend adjustment failure: (file: %s): %s\n"),
			file.c_str(),
			strerror(errno));
		return false;
	}

	int written = write(fd, value.data(), value.size());
	int error = errno;
	close(fd);

	if( written < 0 || (size_t)written != value.size() ) {
		printf(_("autosuspend adjustment failure (write): (file: %s): %s\n"),
			file.c_str(),
			strerror(error));
		return false;
	}

	printf(_("autosuspend adjustment: wrote %s to %s\n"),
		value.c_str(), file.c_str());
	return true;
}

//
// Checks for USB suspend, and enables the device if suspended.
//
// Kernel 2.6.21 behaves with autosuspend=0 meaning off, while 2.6.22
// and higher needs autosuspend=-1 to turn it off.  In 2.6.22, a value
// of 0 means "immediate" instead of "never".
//
// Version 2.6.22 adds variables internal to the system called
// autosuspend_disabled and autoresume_disabled.  These are controlled by the
// /sys/class/usb_device/*/device/power/level file.  (See below)
//
// Here's a summary of files under device/power.  These may or may not exist
// depending on your kernel version and configuration.
//
//
//        autosuspend
//                -1 or 0 means off, depending on kernel,
//                otherwise it is the number of seconds to
//                autosuspend
//
//        level
//                with the settings:
//
//                on      - suspend is disabled, device is fully powered
//                auto    - suspend is controlled by the kernel (default)
//                suspend - suspend is enabled permanently
//
//                You can write these strings to the file to control
//                behaviour on a per-device basis.
//
//                echo on > /sys/usb_device/.../device/power/level
//
//        state
//                current state of device
//                0 - fully powered
//                2 - suspended
//
//                You can write these numbers to control behaviour, but
//                any change you make here might change automatically
//                if autosuspend is on.
//
//                echo -n 0 > /sys/usb_device/.../device/power/state
//
//        wakeup
//                unknown
//
//
// Given the above facts, use the following heuristics to try to disable
// autosuspend for the Blackberry:
//
//	- if level exists, write "on" to it
//	- if autosuspend exists, write -1 to it
//		- if error, write 0 to it
//	- if neither of the above work, and state exists, write 0 to it
//
void resume()
{
	if( udev_devpath.size() == 0 )
		return;	// nothing to do

	// let sysfs settle a bit
	sleep(5);

	std::string power_path = sysfs_path + "/" + udev_devpath + "/device/power/";

	if( !power_write(power_path + "level", "on\n") )
		if( !power_write(power_path + "autosuspend", "-1\n") )
			if( !power_write(power_path + "autosuspend", "0\n") )
				power_write(power_path + "state", "0");
}

int main(int argc, char *argv[])
{
	struct usb_bus *busses;
	ModeType mode = NO_CHANGE;

	INIT_I18N(PACKAGE);

	//
	// allow -o command line switch to choose which mode to use for
	// Blackberry Pearls:
	//	Dual:           0004	-d
	//	With switch:    0001	-o
	//

	// process command line options
	for(;;) {
		int cmd = getopt(argc, argv, "dogp:s:h");
		if( cmd == -1 )
			break;

		switch( cmd )
		{
		case 'd':	// Dual
			mode = PEARL_DUAL_MODE_0004;
			break;

		case 'o':	// Classic style pearl
			mode = PEARL_CLASSIC_MODE_0001;
			break;

		case 'g':	// Guess whether dual is needed
			mode = CONDITIONAL_DUAL_MODE;
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
		printf(_("\nUnable to scan devices: %s\n"), usb_strerror());
		return 1;
	}
	busses = usb_get_busses();

	printf(_("Scanning for Blackberry devices...\n"));

	struct usb_bus *bus;
	for( bus = busses; bus; bus = bus->next ) {
		struct usb_device *dev;

		for (dev = bus->devices; dev; dev = dev->next) {
			// Is this a blackberry?
			if( dev->descriptor.idVendor == VENDOR_RIM ) {
				if( !process(dev, mode) )
					resume();
			}
		}
	}
}

