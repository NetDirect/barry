#!/bin/sh
# $Id$
#
# Unbinding usb-storage from the BlackBarry.
#
#   Copyright (C) 2008  Niels de Vos <nixpanic@users.sourceforge.net>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#
# Disconnecting the BlackBerry from usb-storage is needed as btool needs to
# change the active USB-configuration.
#
# This script searched for any usb-storage devices and disconnects the
# device from the usb-storage driver. The driver kept loaded and therefore
# the barry-tools do not influence other usb-storage devices anymore.
#
# To enable debugging, use "export DEBUG=1" before execting this script.
#

# vendor as deteced by sd_mod (regex regex for egrep)
SD_VENDOR='^RIM[[:space:]]*$'
# vendor as detected by usb-vendorId (regex for egrep)
USB_VENDOR='^0fca$'

# the sysfs-file for disconnecting the device from usb-storage
USB_STORAGE_UNBIND='/sys/bus/usb/drivers/usb-storage/unbind'


# enable debugging with "export DEBUG=1"
function debug()
{
	[ -n "${DEBUG}" ] && echo "${@}" >&2
	return 0
}


# find all vendor files (SCSI-specific) for USB-devices -> usb-storage
USB_STORAGE_VENDORS=$(find /sys/devices -name vendor -path '*/usb*/host*/target*')

for USB_STORAGE_VENDOR in ${USB_STORAGE_VENDORS}
do
	# check if the file contains 'RIM' (file provided by sd_mod)
	if ! egrep -q "${SD_VENDOR}" "${USB_STORAGE_VENDOR}"
	then
		debug "device is not produced by ${SD_VENDOR}"
		continue
	fi

	# find the vendorId (file provided by usbcore)
	# a subdirectory on the same level as idVendor contains the important
	# usb-device-id (the id looks like 4-1:1.0)
	USB_IDVENDOR="${USB_STORAGE_VENDOR}"
	while [ ! -e "${USB_IDVENDOR}/../idVendor" ]
	do
		# go on with the parent directory
		USB_IDVENDOR=$(dirname "${USB_IDVENDOR}")
	done
	# the unique usb-device-id (for usb-storage) is on this level
	USB_DEVID=$(basename "${USB_IDVENDOR}")
	USB_IDVENDOR="${USB_IDVENDOR}/../idVendor"

	# check if the file contains '0fca' (the RIM USB-vendorId)
	if ! egrep -q "${USB_VENDOR}" "${USB_IDVENDOR}"
	then
		debug "device does not have USB-vendorId ${USB_VENDOR}"
		break
	fi

	# found+verified all needed info, now do the disconnect!
	debug "going to unbind ${USB_DEVID} from usb-storage"
	echo -n "${USB_DEVID}" > "${USB_STORAGE_UNBIND}"
done

