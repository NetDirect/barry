///
/// \file	usbwrap.cc
///		USB API wrapper
///

/*
    Copyright (C) 2005-2009, Chris Frey

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


#include "usbwrap.h"
#include "data.h"
#include "error.h"
#include "debug.h"

#include <iomanip>
#include <sstream>
#include <errno.h>
#include <string.h>

#ifndef __DEBUG_MODE__
#define __DEBUG_MODE__
#endif
#include "debug.h"

namespace Usb {

///////////////////////////////////////////////////////////////////////////////
// Usb::Error exception class

static std::string GetErrorString(int libusb_errcode, const std::string &str)
{
	std::ostringstream oss;
	oss << "(";
	
	if( libusb_errcode ) {
		oss << std::setbase(10) << libusb_errcode << ", ";
	}

//	oss << strerror(-libusb_errno) << "): "
	oss << usb_strerror() << "): ";
	oss << str;
	return oss.str();
}

Error::Error(const std::string &str)
	: Barry::Error(GetErrorString(0, str))
	, m_libusb_errcode(0)
{
}

Error::Error(int libusb_errcode, const std::string &str)
	: Barry::Error(GetErrorString(libusb_errcode, str))
	, m_libusb_errcode(libusb_errcode)
{
}


///////////////////////////////////////////////////////////////////////////////
// Match

Match::Match(int vendor, int product,
		const char *busname, const char *devname)
	: m_busses(0)
	, m_dev(0)
	, m_vendor(vendor)
	, m_product(product)
	, m_busname(busname)
	, m_devname(devname)
{
	usb_find_busses();
	usb_find_devices();
	m_busses = usb_get_busses();
}

Match::~Match()
{
}

bool Match::ToNum(const char *str, long &num)
{
	char *end = 0;
	num = strtol(str, &end, 10);
	return	num >= 0 &&			// no negative numbers
		num != LONG_MIN && num != LONG_MAX &&	// no overflow
		str != end && *end == '\0';	// whole string valid
}

//
// Linux treats bus and device path names as numbers, sometimes left
// padded with zeros.  Other platforms, such as Windows, use strings,
// such as "bus-1" or similar.
//
// Here we try to convert each string to a number, and if successful,
// compare them.  If unable to convert, then compare as strings.
// This way, "3" == "003" and "bus-foobar" == "bus-foobar".
//
bool Match::NameCompare(const char *n1, const char *n2)
{
	long l1, l2;
	if( ToNum(n1, l1) && ToNum(n2, l2) ) {
		return l1 == l2;
	}
	else {
		return strcmp(n1, n2) == 0;
	}
}

bool Match::next_device(Usb::DeviceIDType *devid)
{
	for( ; m_busses; m_busses = m_busses->next ) {

		// only search on given bus
		if( m_busname && !NameCompare(m_busname, m_busses->dirname) )
			continue;

		if( !m_dev )
			m_dev = m_busses->devices;

		for( ; m_dev; m_dev = m_dev->next ) {

			// search for specific device
			if( m_devname && !NameCompare(m_devname, m_dev->filename) )
				continue;

			// is there a match?
			if( m_dev->descriptor.idVendor == m_vendor &&
			    m_dev->descriptor.idProduct == m_product ) {
				// found!
				*devid = m_dev;

				// advance for next time
				m_dev = m_dev->next;
				if( !m_dev )
					m_busses = m_busses->next;

				// done
				return true;
			}
		}
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// Device

Device::Device(Usb::DeviceIDType id, int timeout)
	: m_id(id),
	m_timeout(timeout)
{
	dout("usb_open(" << std::dec << id << ")");
	m_handle = usb_open(id);
	if( !m_handle )
		throw Error("open failed");
}

Device::~Device()
{
	dout("usb_close(" << std::dec << m_handle << ")");
	usb_close(m_handle);
}

bool Device::SetConfiguration(unsigned char cfg)
{
	dout("usb_set_configuration(" << std::dec << m_handle << "," << std::dec << (unsigned int) cfg << ")");
	int ret = usb_set_configuration(m_handle, cfg);
	m_lasterror = ret;
	return ret >= 0;
}

bool Device::ClearHalt(int ep)
{
	dout("usb_clear_halt(" << std::dec << m_handle << "," << std::dec << ep << ")");
	int ret = usb_clear_halt(m_handle, ep);
	m_lasterror = ret;
	return ret >= 0;
}

bool Device::Reset()
{
	dout("usb_reset(" << std::dec << m_handle << ")");
	int ret = usb_reset(m_handle);
	m_lasterror = ret;
	return ret == 0;
}

bool Device::BulkRead(int ep, Barry::Data &data, int timeout)
{
	int ret;
	do {
		ret = usb_bulk_read(m_handle, ep,
			(char*) data.GetBuffer(), data.GetBufSize(),
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_bulk_read");
			else
				throw Error(ret, "Error in usb_bulk_read");
		}
		data.ReleaseBuffer(ret);
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

bool Device::BulkWrite(int ep, const Barry::Data &data, int timeout)
{
	ddout("BulkWrite to endpoint " << std::dec << ep << ":\n" << data);
	int ret;
	do {
		ret = usb_bulk_write(m_handle, ep,
			(char*) data.GetData(), data.GetSize(),
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_bulk_write");
			else
				throw Error(ret, "Error in usb_bulk_write");
		}
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

bool Device::BulkWrite(int ep, const void *data, size_t size, int timeout)
{
#ifdef __DEBUG_MODE__
	Barry::Data dump(data, size);
	ddout("BulkWrite to endpoint " << std::dec << ep << ":\n" << dump);
#endif

	int ret;
	do {
		ret = usb_bulk_write(m_handle, ep,
			(char*) data, size,
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_bulk_read");
			else
				throw Error(ret, "Error in usb_bulk_read");
		}
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

bool Device::InterruptRead(int ep, Barry::Data &data, int timeout)
{
	int ret;
	do {
		ret = usb_interrupt_read(m_handle, ep,
			(char*) data.GetBuffer(), data.GetBufSize(),
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_bulk_read");
			else
				throw Error(ret, "Error in usb_bulk_read");
		}
		data.ReleaseBuffer(ret);
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

bool Device::InterruptWrite(int ep, const Barry::Data &data, int timeout)
{
	ddout("InterruptWrite to endpoint " << std::dec << ep << ":\n" << data);

	int ret;
	do {
		ret = usb_interrupt_write(m_handle, ep,
			(char*) data.GetData(), data.GetSize(),
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_bulk_read");
			else
				throw Error(ret, "Error in usb_bulk_read");
		}
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

//
// BulkDrain
//
/// Reads anything available on the given endpoint, with a low timeout,
/// in order to clear any pending reads.
///
void Device::BulkDrain(int ep, int timeout)
{
	try {
		Barry::Data data;
		while( BulkRead(ep, data, timeout) )
		;
	}
	catch( Usb::Error & ) {}
}

//
// GetConfiguration
//
/// Uses the GET_CONFIGURATION control message to determine the currently
/// selected USB configuration, returning it in the cfg argument.
/// If unsuccessful, returns false.
///
bool Device::GetConfiguration(unsigned char &cfg)
{
	int result = usb_control_msg(m_handle, 0x80, USB_REQ_GET_CONFIGURATION, 0, 0,
		(char*) &cfg, 1, m_timeout);
	m_lasterror = result;
	return result >= 0;
}



///////////////////////////////////////////////////////////////////////////////
// Interface

Interface::Interface(Device &dev, int iface)
	: m_dev(dev), m_iface(iface)
{
	dout("usb_claim_interface(" << dev.GetHandle() << "," << std::dec << iface << ")");
	int ret = usb_claim_interface(dev.GetHandle(), iface);
	if( ret < 0 )
		throw Error(ret, "claim interface failed");
}

Interface::~Interface()
{
	dout("usb_release_interface(" << m_dev.GetHandle() << "," << std::dec << m_iface << ")");
	usb_release_interface(m_dev.GetHandle(), m_iface);
}



///////////////////////////////////////////////////////////////////////////////
// EndpointDiscovery

bool EndpointDiscovery::Discover(struct usb_interface_descriptor *interface, int epcount)
{
	// start fresh
	clear();
	m_valid = false;

	EndpointPair pair;

	if( !interface || !interface->endpoint ) {
		dout("EndpointDiscovery::Discover: empty interface pointer");
		return false;
	}

	for( int i = 0; i < epcount; i++ ) {
		// load descriptor
		usb_endpoint_descriptor desc;
		desc = interface->endpoint[i];
		dout("      endpoint_desc #" << i << " loaded"
			<< "\nbLength: " << (unsigned ) desc.bLength
			<< "\nbDescriptorType: " << (unsigned ) desc.bDescriptorType
			<< "\nbEndpointAddress: " << (unsigned ) desc.bEndpointAddress
			<< "\nbmAttributes: " << (unsigned ) desc.bmAttributes
			<< "\nwMaxPacketSize: " << (unsigned ) desc.wMaxPacketSize
			<< "\nbInterval: " << (unsigned ) desc.bInterval
			<< "\nbRefresh: " << (unsigned ) desc.bRefresh
			<< "\nbSynchAddress: " << (unsigned ) desc.bSynchAddress
			<< "\n"
			);

		// add to the map
		(*this)[desc.bEndpointAddress] = desc;
		dout("      endpoint added to map with bEndpointAddress: " << (unsigned int)desc.bEndpointAddress);

		// parse the endpoint into read/write sets, if possible,
		// going in discovery order...
		// Assumptions:
		//	- endpoints of related utility will be grouped
		//	- endpoints with same type will be grouped
		//	- endpoints that do not meet the above assumptions
		//		do not belong in a pair
		unsigned char type = desc.bmAttributes & USB_ENDPOINT_TYPE_MASK;
		if( desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK ) {
			// read endpoint
			pair.read = desc.bEndpointAddress;
			dout("        pair.read = " << (unsigned int)pair.read);
			if( pair.IsTypeSet() && pair.type != type ) {
				// if type is already set, we must start over
				pair.write = 0;
			}
		}
		else {
			// write endpoint
			pair.write = desc.bEndpointAddress;
			dout("        pair.write = " << (unsigned int)pair.write);
			if( pair.IsTypeSet() && pair.type != type ) {
				// if type is already set, we must start over
				pair.read = 0;
			}
		}
		// save the type last
		pair.type = type;
		dout("        pair.type = " << (unsigned int)pair.type);

		// if pair is complete, add to array
		if( pair.IsComplete() ) {
			m_endpoints.push_back(pair);
			dout("        pair added! ("
				<< "read: " << (unsigned int)pair.read << ","
				<< "write: " << (unsigned int)pair.write << ","
				<< "type: " << (unsigned int)pair.type << ")");
			pair = EndpointPair();	// clear
		}
	}

	// just for debugging purposes, check for extra descriptors, and
	// dump them to dout if they exist
	if( interface->extra ) {
		dout("while parsing endpoints, found a block of extra descriptors:");
		Barry::Data data(interface->extra, interface->extralen);
		dout(data);
	}

	return m_valid = true;
}


///////////////////////////////////////////////////////////////////////////////
// InterfaceDiscovery

bool InterfaceDiscovery::DiscoverInterface(struct usb_interface *interface)
{
	if( !interface->altsetting ) {
		dout("InterfaceDiscovery::DiscoverIterface: empty altsetting");
		// some devices are buggy and return a higher bNumInterfaces
		// than the number of interfaces available... in this case
		// we just skip and continue
		return true;
	}

	for( int i = 0; i < interface->num_altsetting; i++ ) {
		// load descriptor
		InterfaceDesc desc;
		desc.desc = interface->altsetting[i];
		dout("    interface_desc #" << i << " loaded"
			<< "\nbLength: " << (unsigned) desc.desc.bLength
			<< "\nbDescriptorType: " << (unsigned) desc.desc.bDescriptorType
			<< "\nbInterfaceNumber: " << (unsigned) desc.desc.bInterfaceNumber
			<< "\nbAlternateSetting: " << (unsigned) desc.desc.bAlternateSetting
			<< "\nbNumEndpoints: " << (unsigned) desc.desc.bNumEndpoints
			<< "\nbInterfaceClass: " << (unsigned) desc.desc.bInterfaceClass
			<< "\nbInterfaceSubClass: " << (unsigned) desc.desc.bInterfaceSubClass
			<< "\nbInterfaceProtocol: " << (unsigned) desc.desc.bInterfaceProtocol
			<< "\niInterface: " << (unsigned) desc.desc.iInterface
			<< "\n"
			);

		// load all endpoints on this interface
		if( !desc.endpoints.Discover(&desc.desc, desc.desc.bNumEndpoints) ) {
			dout("    endpoint discovery failed for bInterfaceNumber: " << (unsigned int)desc.desc.bInterfaceNumber << ", not added to map.");
			return false;
		}

		// add to the map
		(*this)[desc.desc.bInterfaceNumber] = desc;
		dout("    interface added to map with bInterfaceNumber: " << (unsigned int)desc.desc.bInterfaceNumber);
	}
	return true;
}

bool InterfaceDiscovery::Discover(Usb::DeviceIDType devid, int cfgidx, int ifcount)
{
	// start fresh
	clear();
	m_valid = false;

	if( !devid || !devid->config || !devid->config[cfgidx].interface ) {
		dout("InterfaceDiscovery::Discover: empty devid/config/interface");
		return false;
	}

	for( int i = 0; i < ifcount; i++ ) {
		if( !DiscoverInterface(&devid->config[cfgidx].interface[i]) )
			return false;
	}

	return m_valid = true;
}


///////////////////////////////////////////////////////////////////////////////
// ConfigDiscovery

bool ConfigDiscovery::Discover(Usb::DeviceIDType devid, int cfgcount)
{
	// start fresh
	clear();
	m_valid = false;

	for( int i = 0; i < cfgcount; i++ ) {
		// load descriptor
		ConfigDesc desc;
		if( !devid || !devid->config ) {
			dout("ConfigDiscovery::Discover: empty devid or config");
			return false;
		}
		desc.desc = devid->config[i];
		dout("  config_desc #" << i << " loaded"
			<< "\nbLength: " << (unsigned int) desc.desc.bLength
			<< "\nbDescriptorType: " << (unsigned int) desc.desc.bDescriptorType
			<< "\nwTotalLength: " << (unsigned int) desc.desc.wTotalLength
			<< "\nbNumInterfaces: " << (unsigned int) desc.desc.bNumInterfaces
			<< "\nbConfigurationValue: " << (unsigned int) desc.desc.bConfigurationValue
			<< "\niConfiguration: " << (unsigned int) desc.desc.iConfiguration
			<< "\nbmAttributes: " << (unsigned int) desc.desc.bmAttributes
			<< "\nMaxPower: " << (unsigned int) desc.desc.MaxPower
			<< "\n"
			);

		// just for debugging purposes, check for extra descriptors, and
		// dump them to dout if they exist
		if( desc.desc.extra ) {
			dout("while parsing config descriptor, found a block of extra descriptors:");
			Barry::Data data(desc.desc.extra, desc.desc.extralen);
			dout(data);
		}

		// load all interfaces on this configuration
		if( !desc.interfaces.Discover(devid, i, desc.desc.bNumInterfaces) ) {
			dout("  config discovery failed for bConfigurationValue: " << (unsigned int)desc.desc.bConfigurationValue << ", not added to map.");
			return false;
		}

		// add to the map
		(*this)[desc.desc.bConfigurationValue] = desc;
		dout("  config added to map with bConfigurationValue: " << (unsigned int)desc.desc.bConfigurationValue);
	}

	return m_valid = true;
}


///////////////////////////////////////////////////////////////////////////////
// DeviceDiscovery

DeviceDiscovery::DeviceDiscovery(Usb::DeviceIDType devid)
	: m_valid(false)
{
	Discover(devid);
}

bool DeviceDiscovery::Discover(Usb::DeviceIDType devid)
{
	// start fresh
	configs.clear();
	m_valid = false;

	// copy the descriptor over to our memory
	if( !devid ) {
		dout("DeviceDiscovery::Discover: empty devid");
		return false;
	}

	desc = devid->descriptor;
	dout("device_desc loaded"
		<< "\nbLength: " << (unsigned int) desc.bLength
		<< "\nbDescriptorType: " << (unsigned int) desc.bDescriptorType
		<< "\nbcdUSB: " << (unsigned int) desc.bcdUSB
		<< "\nbDeviceClass: " << (unsigned int) desc.bDeviceClass
		<< "\nbDeviceSubClass: " << (unsigned int) desc.bDeviceSubClass
		<< "\nbDeviceProtocol: " << (unsigned int) desc.bDeviceProtocol
		<< "\nbMaxPacketSize0: " << (unsigned int) desc.bMaxPacketSize0
		<< "\nidVendor: " << (unsigned int) desc.idVendor
		<< "\nidProduct: " << (unsigned int) desc.idProduct
		<< "\nbcdDevice: " << (unsigned int) desc.bcdDevice
		<< "\niManufacturer: " << (unsigned int) desc.iManufacturer
		<< "\niProduct: " << (unsigned int) desc.iProduct
		<< "\niSerialNumber: " << (unsigned int) desc.iSerialNumber
		<< "\nbNumConfigurations: " << (unsigned int) desc.bNumConfigurations
		<< "\n"
	);

	m_valid = configs.Discover(devid, desc.bNumConfigurations);
	return m_valid;
}

} // namespace Usb

