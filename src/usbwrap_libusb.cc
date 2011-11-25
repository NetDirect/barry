///
/// \file	usbwrap_libusb.cc
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

#include "usbwrap_libusb.h"

#include "debug.h"
#include "data.h"
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <algorithm>

#ifndef __DEBUG_MODE__
#define __DEBUG_MODE__
#endif
#include "debug.h"

namespace Usb {

// helper function to make deleting pointers in maps and vectors easier
template<typename T> static void deletePtr(T* ptr) {
	delete ptr;
}

template<typename K, typename T> static void deleteMapPtr(std::pair<K,T*> ptr) {
	delete ptr.second;
}

///////////////////////////////////////////////////////////////////////////////
// Static functions

std::string LibraryInterface::GetLastErrorString(int /*libusb_errcode*/)
{
	// Errcode is unused by libusb, so just call the last error
	return std::string(usb_strerror());
}

int LibraryInterface::TranslateErrcode(int libusb_errcode)
{
	// libusb errcode == system errcode
	return libusb_errcode;
}

bool LibraryInterface::Init(int *libusb_errno)
{
	// if the environment variable USB_DEBUG is set, that
	// level value will be used instead of our 9 below...
	// if you need to *force* this to 9, call SetDataDump(true)
	// after Init()
	usb_init();
	// Can never fail, so return success
	return true;
}

void LibraryInterface::Uninit()
{
	// Nothing to do
}

void LibraryInterface::SetDataDump(bool data_dump_mode)
{
	if( data_dump_mode )
		usb_set_debug(9);
	else
		usb_set_debug(0);
}

///////////////////////////////////////////////////////////////////////////////
// DeviceID

DeviceID::DeviceID(DeviceIDImpl* impl)
	: m_impl(impl)
{
}

DeviceID::~DeviceID()
{
}

const char* DeviceID::GetBusName() const
{
        return m_impl->m_dev->bus->dirname;
}

uint16_t DeviceID::GetNumber() const
{
	return m_impl->m_dev->devnum;
}

const char* DeviceID::GetFilename() const
{
	return m_impl->m_dev->filename;
}

uint16_t DeviceID::GetIdProduct() const
{
	return m_impl->m_dev->descriptor.idProduct;
}

///////////////////////////////////////////////////////////////////////////////
// DeviceList

DeviceList::DeviceList()
	: m_impl(new DeviceListImpl())
{
	// Work out what devices are on the bus at the moment
	usb_find_busses();
	usb_find_devices();
	struct usb_bus* busses = usb_get_busses();
	for( ; busses; busses = busses->next ) {
		struct usb_device* dev = busses->devices;
		for( ; dev; dev = dev->next ) {
			// Add the device to the list of devices
			std::auto_ptr<DeviceIDImpl> impl( new DeviceIDImpl() );
			impl->m_dev = dev;
			DeviceID devID(impl.release());
			m_impl->m_devices.push_back(devID);
		}
	}
}

DeviceList::~DeviceList()
{
}


static bool ToNum(const char *str, long &num)
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
static bool NameCompare(const char *n1, const char *n2)
{
	long l1, l2;
	if( ToNum(n1, l1) && ToNum(n2, l2) ) {
		return l1 == l2;
	}
	else {
		return strcmp(n1, n2) == 0;
	}
}

std::vector<DeviceID> DeviceList::MatchDevices(int vendor, int product,
					    const char *busname, const char *devname)
{
	std::vector<DeviceID> ret;
	
	std::vector<DeviceID>::iterator iter = m_impl->m_devices.begin();

	for( ; iter != m_impl->m_devices.end() ; ++iter ) {
		struct usb_device* dev = iter->m_impl->m_dev;
		
		// only search on given bus
		if( busname && !NameCompare(busname, dev->bus->dirname) )
			continue;
		
		// search for specific device
		if( devname && !NameCompare(devname, dev->filename) )
			continue;

		// is there a match?
		if( dev->descriptor.idVendor == vendor &&
		    ( dev->descriptor.idProduct == product ||
		      product == PRODUCT_ANY )) {
			ret.push_back(*iter);
		}
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Device

Device::Device(const Usb::DeviceID& id, int timeout)
	: m_id(id),
	m_timeout(timeout)
{
	dout("usb_open(" << std::dec << id.m_impl.get() << ")");
	if( !id.m_impl.get() )
		throw Error("invalid USB device ID");
	m_handle.reset(new DeviceHandle());
	m_handle->m_handle = usb_open(id.m_impl->m_dev);
	if( !m_handle->m_handle )
		throw Error("open failed");
}

Device::~Device()
{
	dout("usb_close(" << std::dec << m_handle->m_handle << ")");
	usb_close(m_handle->m_handle);
}

bool Device::SetConfiguration(unsigned char cfg)
{
	dout("usb_set_configuration(" << std::dec << m_handle->m_handle << ", 0x" << std::hex << (unsigned int) cfg << ")");
	int ret = usb_set_configuration(m_handle->m_handle, cfg);
	m_lasterror = ret;
	return ret >= 0;
}

bool Device::ClearHalt(int ep)
{
	dout("usb_clear_halt(" << std::dec << m_handle->m_handle << ", 0x" << std::hex << ep << ")");
	int ret = usb_clear_halt(m_handle->m_handle, ep);
	m_lasterror = ret;
	return ret >= 0;
}

bool Device::Reset()
{
	dout("usb_reset(" << std::dec << m_handle->m_handle << ")");
	int ret = usb_reset(m_handle->m_handle);
	m_lasterror = ret;
	return ret == 0;
}

bool Device::BulkRead(int ep, Barry::Data &data, int timeout)
{
	int ret;
	do {
		data.QuickZap();
		ret = usb_bulk_read(m_handle->m_handle, ep,
			(char*) data.GetBuffer(), data.GetBufSize(),
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_bulk_read");
			else {
				std::ostringstream oss;
				oss << "Error in usb_bulk_read("
				    << m_handle->m_handle << ", "
				    << ep << ", buf, "
				    << data.GetBufSize() << ")";
				throw Error(ret, oss.str());
			}
		}
		else if( ret > 0 )
			data.ReleaseBuffer(ret);
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

bool Device::BulkWrite(int ep, const Barry::Data &data, int timeout)
{
	ddout("BulkWrite to endpoint 0x" << std::hex << ep << ":\n" << data);
	int ret;
	do {
		ret = usb_bulk_write(m_handle->m_handle, ep,
			(char*) data.GetData(), data.GetSize(),
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_bulk_write (1)");
			else
				throw Error(ret, "Error in usb_bulk_write (1)");
		}
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

bool Device::BulkWrite(int ep, const void *data, size_t size, int timeout)
{
#ifdef __DEBUG_MODE__
	Barry::Data dump(data, size);
	ddout("BulkWrite to endpoint 0x" << std::hex << ep << ":\n" << dump);
#endif

	int ret;
	do {
		ret = usb_bulk_write(m_handle->m_handle, ep,
			(char*) data, size,
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_bulk_write (2)");
			else
				throw Error(ret, "Error in usb_bulk_write (2)");
		}
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

bool Device::InterruptRead(int ep, Barry::Data &data, int timeout)
{
	int ret;
	do {
		data.QuickZap();
		ret = usb_interrupt_read(m_handle->m_handle, ep,
			(char*) data.GetBuffer(), data.GetBufSize(),
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_interrupt_read");
			else
				throw Error(ret, "Error in usb_interrupt_read");
		}
		else if( ret > 0 )
			data.ReleaseBuffer(ret);
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

bool Device::InterruptWrite(int ep, const Barry::Data &data, int timeout)
{
	ddout("InterruptWrite to endpoint 0x" << std::hex << ep << ":\n" << data);

	int ret;
	do {
		ret = usb_interrupt_write(m_handle->m_handle, ep,
			(char*) data.GetData(), data.GetSize(),
			timeout == -1 ? m_timeout : timeout);
		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in usb_interrupt_write");
			else
				throw Error(ret, "Error in usb_interrupt_write");
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
	int result = usb_control_msg(m_handle->m_handle, 0x80, USB_REQ_GET_CONFIGURATION, 0, 0,
		(char*) &cfg, 1, m_timeout);
	m_lasterror = result;
	return result >= 0;
}

// Returns the current power level of the device, or 0 if unknown
int Device::GetPowerLevel()
{
	if( !m_id.m_impl->m_dev->config ||
	    !m_id.m_impl->m_dev->descriptor.bNumConfigurations < 1 )
		return 0;

	return m_id.m_impl->m_dev->config[0].MaxPower;
}

bool Device::IsAttachKernelDriver(int iface, std::string& name) 
{
	int ret;
	char buffer[64];

	ret = usb_get_driver_np(m_handle->m_handle, iface, buffer, sizeof(buffer));
	if (ret == 0) {
		dout("interface (" << m_handle->m_handle << ", 0x" << std::hex << iface 
			<< ") already claimed by driver \"" << name << "\" "
			<< "attempting to detach it ");
		name = buffer;
		return true;
	}

	return false;
}

// Requests that the kernel driver is detached, returning false on failure
bool Device::DetachKernelDriver(int iface)
{
#if LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	int result = usb_detach_kernel_driver_np(m_handle->m_handle, iface);
	m_lasterror = result;
	return result >= 0;
#else
	m_lasterror = -ENOSYS;
	return false;
#endif
}

// Sends a control message to the device, returning false on failure
bool Device::ControlMsg(int requesttype, int request, int value,
			int index, char *bytes, int size, int timeout)
{
	int result = usb_control_msg(m_handle->m_handle,
				     requesttype, request, value, index,
				     bytes, size, timeout);
	m_lasterror = result;
	return result >= 0;
}


int Device::FindInterface(int ifaceClass)
{
	struct usb_config_descriptor *cfg = m_id.m_impl->m_dev->config;

	if( cfg ) {

		for( unsigned i = 0; cfg->interface && i < cfg->bNumInterfaces; i++ ) {
			struct usb_interface *iface = &cfg->interface[i];
			for( int a = 0; iface->altsetting && a < iface->num_altsetting; a++ ) {
				struct usb_interface_descriptor *id = &iface->altsetting[a];
				if( id->bInterfaceClass == ifaceClass )
					return id->bInterfaceNumber;
			}
		}
	}

	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// Interface

Interface::Interface(Device &dev, int iface)
	: m_dev(dev), m_iface(iface)
{
	dout("usb_claim_interface(" << dev.GetHandle()->m_handle << ", 0x" << std::hex << iface << ")");
	int ret = usb_claim_interface(dev.GetHandle()->m_handle, iface);
	if( ret < 0 )
		throw Error(ret, "claim interface failed");
}

Interface::~Interface()
{
	dout("usb_release_interface(" << m_dev.GetHandle()->m_handle << ", 0x" << std::hex << m_iface << ")");
	usb_release_interface(m_dev.GetHandle()->m_handle, m_iface);
}

//
// SetAltInterface
//
/// Uses the usb_set_altinterface() function to set the currently
/// selected USB alternate setting of the current interface.
/// The iface parameter passed in should be a value specified
/// in the bAlternateSetting descriptor field.
/// If unsuccessful, returns false.
///
bool Interface::SetAltInterface(int altSetting)
{
	int result = usb_set_altinterface(m_dev.GetHandle()->m_handle, altSetting);
	m_dev.SetLastError(result);
	return result >= 0;
}

//////////////////////////////////////////////////////////////////
// DeviceDescriptor

DeviceDescriptor::DeviceDescriptor(DeviceID& devid)
	: m_impl(new DeviceDescriptorImpl())
{
	if( !devid.m_impl.get() ) {
		dout("DeviceDescriptor: empty devid");
		return;
	}
	// Copy the descriptor over to our memory
	m_impl->m_dev = devid.m_impl->m_dev;
	m_impl->m_desc = devid.m_impl->m_dev->descriptor;
	dout("device_desc loaded"
	     << "\nbLength: " << std::dec << (unsigned int) m_impl->m_desc.bLength
	     << "\nbDescriptorType: " << std::dec << (unsigned int) m_impl->m_desc.bDescriptorType
	     << "\nbcdUSB: 0x" << std::hex << (unsigned int) m_impl->m_desc.bcdUSB
	     << "\nbDeviceClass: " << std::dec << (unsigned int) m_impl->m_desc.bDeviceClass
	     << "\nbDeviceSubClass: " << std::dec << (unsigned int) m_impl->m_desc.bDeviceSubClass
	     << "\nbDeviceProtocol: " << std::dec << (unsigned int) m_impl->m_desc.bDeviceProtocol
	     << "\nbMaxPacketSize0: " << std::dec << (unsigned int) m_impl->m_desc.bMaxPacketSize0
	     << "\nidVendor: 0x" << std::hex << (unsigned int) m_impl->m_desc.idVendor
	     << "\nidProduct: 0x" << std::hex << (unsigned int) m_impl->m_desc.idProduct
	     << "\nbcdDevice: 0x" << std::hex << (unsigned int) m_impl->m_desc.bcdDevice
	     << "\niManufacturer: " << std::dec << (unsigned int) m_impl->m_desc.iManufacturer
	     << "\niProduct: " << std::dec << (unsigned int) m_impl->m_desc.iProduct
	     << "\niSerialNumber: " << std::dec << (unsigned int) m_impl->m_desc.iSerialNumber
	     << "\nbNumConfigurations: " << std::dec << (unsigned int) m_impl->m_desc.bNumConfigurations
	     << "\n"
	);
	
	// Create all the configs
	for( int i = 0; i < m_impl->m_desc.bNumConfigurations; ++i ) {
		std::auto_ptr<ConfigDescriptor> ptr(new ConfigDescriptor(*this, i));
		(*this)[ptr->GetNumber()] = ptr.get();
		ptr.release();
	}
}

DeviceDescriptor::~DeviceDescriptor()
{
	// Delete any pointers in the map
	std::for_each(begin(),
		      end(),
		      deleteMapPtr<int, ConfigDescriptor>);
}

///////////////////////////////////////////////////////////////////
// ConfigDescriptor

ConfigDescriptor::ConfigDescriptor(DeviceDescriptor& dev, int cfgnumber)
	: m_impl(new ConfigDescriptorImpl())
{
	// Copy the config descriptor locally
	m_impl->m_desc = dev.m_impl->m_dev->config[cfgnumber];
	dout("  config_desc #" << std::dec << cfgnumber << " loaded"
	     << "\nbLength: " << std::dec << (unsigned int) m_impl->m_desc.bLength
	     << "\nbDescriptorType: " << std::dec << (unsigned int) m_impl->m_desc.bDescriptorType
	     << "\nwTotalLength: " << std::dec << (unsigned int) m_impl->m_desc.wTotalLength
	     << "\nbNumInterfaces: " << std::dec << (unsigned int) m_impl->m_desc.bNumInterfaces
	     << "\nbConfigurationValue: " << std::dec << (unsigned int) m_impl->m_desc.bConfigurationValue
	     << "\niConfiguration: " << std::dec << (unsigned int) m_impl->m_desc.iConfiguration
	     << "\nbmAttributes: 0x" << std::hex << (unsigned int) m_impl->m_desc.bmAttributes
	     << "\nMaxPower: " << std::dec << (unsigned int) m_impl->m_desc.MaxPower
	     << "\n"
		);

	// just for debugging purposes, check for extra descriptors, and
	// dump them to dout if they exist
	if( m_impl->m_desc.extra ) {
		dout("while parsing config descriptor, found a block of extra descriptors:");
		Barry::Data data(m_impl->m_desc.extra, m_impl->m_desc.extralen);
		dout(data);
	}

	// Create all the interfaces
	for( int i = 0; i < m_impl->m_desc.bNumInterfaces; ++i ) {
		struct usb_interface* interface = &(m_impl->m_desc.interface[i]);
		if( !interface->altsetting ) {
			dout("ConfigDescriptor: empty altsetting");
			// some devices are buggy and return a higher bNumInterfaces
			// than the number of interfaces available... in this case
			// we just skip and continue
			continue;
		}
		for( int j = 0; j < interface->num_altsetting; ++j ) {
			std::auto_ptr<InterfaceDescriptor> ptr(
				new InterfaceDescriptor(*this, i, j));
			(*this)[ptr->GetNumber()] = ptr.get();
			ptr.release();
		}
	}
}

ConfigDescriptor::~ConfigDescriptor()
{
	// Delete any pointers in the map
	std::for_each(begin(),
		      end(),
		      deleteMapPtr<int, InterfaceDescriptor>);
}

uint8_t ConfigDescriptor::GetNumber() const {
	return m_impl->m_desc.bConfigurationValue;
}

/////////////////////////////////////////////////////////////////////////
// InterfaceDescriptor

InterfaceDescriptor::InterfaceDescriptor(ConfigDescriptor& cfg,
					 int interface, int altsetting)
	: m_impl(new InterfaceDescriptorImpl())
{
	// Copy the descriptor
	m_impl->m_desc = cfg.m_impl->m_desc
		     .interface[interface]
		     .altsetting[altsetting];
	dout("    interface_desc #" << std::dec << interface << " loaded"
	     << "\nbLength: " << std::dec << (unsigned) m_impl->m_desc.bLength
	     << "\nbDescriptorType: " << std::dec << (unsigned) m_impl->m_desc.bDescriptorType
	     << "\nbInterfaceNumber: " << std::dec << (unsigned) m_impl->m_desc.bInterfaceNumber
	     << "\nbAlternateSetting: " << std::dec << (unsigned) m_impl->m_desc.bAlternateSetting
	     << "\nbNumEndpoints: " << std::dec << (unsigned) m_impl->m_desc.bNumEndpoints
	     << "\nbInterfaceClass: " << std::dec << (unsigned) m_impl->m_desc.bInterfaceClass
	     << "\nbInterfaceSubClass: " << std::dec << (unsigned) m_impl->m_desc.bInterfaceSubClass
	     << "\nbInterfaceProtocol: " << std::dec << (unsigned) m_impl->m_desc.bInterfaceProtocol
	     << "\niInterface: " << std::dec << (unsigned) m_impl->m_desc.iInterface
	     << "\n"
		);

	if( !m_impl->m_desc.endpoint ) {
		dout("InterfaceDescriptor: empty interface pointer");
		return;
	}

	// Create all the endpoints
	for( int i = 0; i < m_impl->m_desc.bNumEndpoints; ++i ) {
		std::auto_ptr<EndpointDescriptor> ptr (
			new EndpointDescriptor(*this, i));
		this->push_back(ptr.get());
		ptr.release();
	}

	// just for debugging purposes, check for extra descriptors, and
	// dump them to dout if they exist
	if( m_impl->m_desc.extra ) {
		dout("while parsing interface descriptor, found a block of extra descriptors:");
		Barry::Data data(m_impl->m_desc.extra, m_impl->m_desc.extralen);
		dout(data);
	}
}

InterfaceDescriptor::~InterfaceDescriptor()
{
	// Delete any pointers in the vector
	std::for_each(begin(),
		      end(),
		      deletePtr<EndpointDescriptor>);
}

uint8_t InterfaceDescriptor::GetClass() const
{
	return m_impl->m_desc.bInterfaceClass;
}

uint8_t InterfaceDescriptor::GetNumber() const
{
	return m_impl->m_desc.bInterfaceNumber;
}

uint8_t InterfaceDescriptor::GetAltSetting() const
{
	return m_impl->m_desc.bAlternateSetting;
}

/////////////////////////////////////////////////////////////////////////////////
// EndpointDescriptor

EndpointDescriptor::EndpointDescriptor(InterfaceDescriptor& interface, int endpoint)
	: m_impl(new EndpointDescriptorImpl()),
	  m_read(false),
	  m_addr(0),
	  m_type(InvalidType)
{
	// Copy the descriptor
	m_impl->m_desc = interface.m_impl->m_desc.endpoint[endpoint];
	dout("      endpoint_desc #" << std::dec << endpoint << " loaded"
	     << "\nbLength: " << std::dec << (unsigned ) m_impl->m_desc.bLength
	     << "\nbDescriptorType: " << std::dec << (unsigned ) m_impl->m_desc.bDescriptorType
	     << "\nbEndpointAddress: 0x" << std::hex << (unsigned ) m_impl->m_desc.bEndpointAddress
	     << "\nbmAttributes: 0x" << std::hex << (unsigned ) m_impl->m_desc.bmAttributes
	     << "\nwMaxPacketSize: " << std::dec << (unsigned ) m_impl->m_desc.wMaxPacketSize
	     << "\nbInterval: " << std::dec << (unsigned ) m_impl->m_desc.bInterval
	     << "\nbRefresh: " << std::dec << (unsigned ) m_impl->m_desc.bRefresh
	     << "\nbSynchAddress: " << std::dec << (unsigned ) m_impl->m_desc.bSynchAddress
	     << "\n"
		);
	// Set up variables
	m_read = ((m_impl->m_desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK) != 0);
	m_addr = (m_impl->m_desc.bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK);
	int type = (m_impl->m_desc.bmAttributes & USB_ENDPOINT_TYPE_MASK);
	m_type = static_cast<Usb::EndpointDescriptor::EpType>(type);

	// just for debugging purposes, check for extra descriptors, and
	// dump them to dout if they exist
	if( m_impl->m_desc.extra ) {
		dout("while parsing endpoint descriptor, found a block of extra descriptors:");
		Barry::Data data(m_impl->m_desc.extra, m_impl->m_desc.extralen);
		dout(data);
	}
}

EndpointDescriptor::~EndpointDescriptor()
{
}

} // namespace Usb

