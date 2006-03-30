///
/// \file	usbwrap.cc
///		USB API wrapper
///

/*
    Copyright (C) 2005-2006, Chris Frey

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

#define __DEBUG_MODE__
#include "debug.h"

namespace Usb {

IO::IO(Device *dev, libusb_io_handle_t *handle)
	: m_dev(dev),
	m_handle(handle)
{
	if( m_handle )
		dout("IO::IO handle: " << m_handle);
}

IO::IO(Device &dev, libusb_io_handle_t *handle)
	: m_dev(&dev),
	m_handle(handle)
{
	if( m_handle )
		dout("IO::IO handle: " << m_handle);
}

IO::IO(const IO &other)
	: m_dev(other.m_dev),
	m_handle(0)
{
	operator=(other);
}

// only one IO object can ever hold a handle to an io operation
IO& IO::operator=(const IO &other)
{
	if( this == &other )
		return *this;		// nothing to do

	if( m_dev && m_dev != other.m_dev )
		throw UsbError("Copying IO objects that are not "
			"related to the same device.");

	cleanup();
	m_handle = other.m_handle;
	other.m_handle = 0;
	return *this;
}

// cleans up the io handle, making this IO object invalid
void IO::cleanup()
{
	if( IsValid() ) {
		dout("IO::cleanup handle: " << m_handle);

		std::ostringstream oss;
		oss << "cleanup: " << GetEndpoint() << " ";
		if( !IsCompleted() ) {
			// not complete yet, cancel
			oss << " cancel ";
			libusb_io_cancel(m_handle);
		}
		oss << " free";
		dout(oss.str());
		libusb_io_free(m_handle);
		m_handle = 0;
	}
}

// will cancel if not complete, and will always free if valid
IO::~IO()
{
	cleanup();
}

// API wrappers
bool IO::IsCompleted() const
{
	if( !IsValid() )
		return true;

	return libusb_is_io_completed(m_handle) != 0;
}

void IO::Wait()
{
	if( !IsValid() )
		return;			// nothing to wait on
	libusb_io_wait(m_handle);

#ifdef __DEBUG_MODE__
	ddout("IO::Wait(): Endpoint " << GetEndpoint()
		<< " Status "
		<< std::setbase(10) << GetStatus() << std::setbase(16));

	// only dump read endpoints that are successful
	if( (GetEndpoint() & USB_ENDPOINT_DIR_MASK) && GetStatus() >= 0 ) {
		Data dump(GetData(), GetSize());
		ddout(dump);
	}
#endif
}

void IO::Cancel()
{
	if( !IsValid() )
		return;
	if( !IsCompleted() ) {
		// not complete yet, cancel
		libusb_io_cancel(m_handle);
	}
}

int IO::GetStatus() const
{
	if( !IsValid() )
		return -1;
	return libusb_io_comp_status(m_handle);
}

int IO::GetSize() const
{
	if( !IsValid() )
		return 0;
	return libusb_io_xfer_size(m_handle);
}

int IO::GetReqSize() const
{
	if( !IsValid() )
		return 0;
	return libusb_io_req_size(m_handle);
}

int IO::GetEndpoint() const
{
	if( !IsValid() )
		return 0;
	return libusb_io_ep_addr(m_handle);
}

unsigned char * IO::GetData()
{
	if( !IsValid() )
		return 0;
	return libusb_io_data(m_handle);
}

const unsigned char * IO::GetData() const
{
	if( !IsValid() )
		return 0;
	return libusb_io_data(m_handle);
}



///////////////////////////////////////////////////////////////////////////////
// Device

bool Device::SetConfiguration(unsigned char cfg)
{
	m_lasterror = libusb_set_configuration(m_handle, cfg);
	return m_lasterror >= 0;
}

bool Device::ClearHalt(int ep)
{
	m_lasterror = libusb_clear_halt(m_handle, ep);
	return m_lasterror >= 0;
}

bool Device::Reset()
{
	m_lasterror = libusb_reset(m_handle);
	return m_lasterror == 0;
}

void Device::TrackBulkRead(int ep, Data &data)
{
	m_ios.push_back(ABulkRead(ep, data));
}

void Device::TrackBulkWrite(int ep, const Data &data)
{
	m_ios.push_back(ABulkWrite(ep, data));
}

void Device::TrackInterruptRead(int ep, Data &data)
{
	m_ios.push_back(AInterruptRead(ep, data));
}

void Device::TrackInterruptWrite(int ep, const Data &data)
{
	m_ios.push_back(AInterruptWrite(ep, data));
}

// extracts the next completed IO handle, and returns it as an IO object
// The Device object no longer tracks this handle, and if the IO object
// goes out of scope, the handle will be freed.
IO Device::PollCompletions()
{
	// get next completed libusb IO handle
	libusb_io_handle_t *done = libusb_io_poll_completions();
	if( !done )
		return IO::InvalidIO();

	// search for it in our list
	io_list_type::iterator b = m_ios.begin();
	for( ; b != m_ios.end(); b++ ) {
		if( *b == done ) {
			IO io = *b;	// make copy to return
			m_ios.erase(b);	// remove forgotten placeholder
			return io;	// return so user can deal with it
		}
	}

	// not found in our tracked list
	if( libusb_io_dev(done) == m_handle ) {
		// this is an event for our device handle, so ok to return it
		return IO(*this, done);
	}
	else {
		// not our device, ignore it
		return IO::InvalidIO();
	}
}

void Device::ClearIO(int index)
{
	m_ios.erase(m_ios.begin() + index);
}

IO Device::ABulkRead(int ep, Data &data)
{
	libusb_io_handle_t *rd = libusb_submit_bulk_read( GetHandle(), ep,
		data.GetBuffer(), data.GetBufSize(), m_timeout, NULL);
	if( !rd )
		throw UsbError("Error in libusb_submit_bulk_read");

	return IO(*this, rd);
}

IO Device::ABulkWrite(int ep, const Data &data)
{
	ddout("ABulkWrite to endpoint " << ep << ":\n" << data);
	libusb_io_handle_t *wr = libusb_submit_bulk_write(GetHandle(), ep,
		data.GetData(), data.GetSize(), m_timeout, NULL);
	if( !wr )
		throw UsbError("Error in libusb_submit_bulk_write");

	return IO(*this, wr);
}

IO Device::ABulkWrite(int ep, const void *data, size_t size)
{
#ifdef __DEBUG_MODE__
	Data dump(data, size);
	ddout("ABulkWrite to endpoint " << ep << ":\n" << dump);
#endif

	libusb_io_handle_t *wr = libusb_submit_bulk_write(GetHandle(), ep,
		data, size, m_timeout, NULL);
	if( !wr )
		throw UsbError("Error in libusb_submit_bulk_write");

	return IO(*this, wr);
}

IO Device::AInterruptRead(int ep, Data &data)
{
	libusb_io_handle_t *rd = libusb_submit_interrupt_read( GetHandle(), ep,
		data.GetBuffer(), data.GetBufSize(), m_timeout, NULL);
	if( !rd )
		throw UsbError("Error in libusb_submit_interrupt_read");

	return IO(*this, rd);
}

IO Device::AInterruptWrite(int ep, const Data &data)
{
	ddout("AInterruptWrite to endpoint " << ep << ":\n" << data);
	libusb_io_handle_t *wr = libusb_submit_interrupt_write(GetHandle(), ep,
		data.GetData(), data.GetSize(), m_timeout, NULL);
	if( !wr )
		throw UsbError("Error in libusb_submit_interrupt_write");

	return IO(*this, wr);
}



///////////////////////////////////////////////////////////////////////////////
// EndpointDiscovery

bool EndpointDiscovery::Discover(libusb_device_id_t devid, int cfgidx, int ifcidx, int epcount)
{
	// start fresh
	clear();
	m_valid = false;

	EndpointPair pair;

	for( int i = 0; i < epcount; i++ ) {
		// load descriptor
		usb_endpoint_desc desc;
		if( libusb_get_endpoint_desc(devid, cfgidx, ifcidx, i, &desc) < 0 )
			return false;
		dout("      endpoint_desc #" << i << " loaded");

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
				<< (unsigned int)pair.read << ","
				<< (unsigned int)pair.write << ","
				<< (unsigned int)pair.type << ")");
			pair = EndpointPair();	// clear
		}
	}

	return m_valid = true;
}


///////////////////////////////////////////////////////////////////////////////
// InterfaceDiscovery

bool InterfaceDiscovery::Discover(libusb_device_id_t devid, int cfgidx, int ifcount)
{
	// start fresh
	clear();
	m_valid = false;

	for( int i = 0; i < ifcount; i++ ) {
		// load descriptor
		InterfaceDesc desc;
		if( libusb_get_interface_desc(devid, cfgidx, i, &desc.desc) < 0 )
			return false;
		dout("    interface_desc #" << i << " loaded");

		// load all endpoints on this interface
		if( !desc.endpoints.Discover(devid, cfgidx, i, desc.desc.bNumEndpoints) )
			return false;

		// add to the map
		(*this)[desc.desc.bInterfaceNumber] = desc;
		dout("    interface added to map with bInterfaceNumber: " << (unsigned int)desc.desc.bInterfaceNumber);
	}

	return m_valid = true;
}


///////////////////////////////////////////////////////////////////////////////
// ConfigDiscovery

bool ConfigDiscovery::Discover(libusb_device_id_t devid, int cfgcount)
{
	// start fresh
	clear();
	m_valid = false;

	for( int i = 0; i < cfgcount; i++ ) {
		// load descriptor
		ConfigDesc desc;
		if( libusb_get_config_desc(devid, i, &desc.desc) < 0 )
			return false;
		dout("  config_desc #" << i << " loaded");

		// load all interfaces on this configuration
		if( !desc.interfaces.Discover(devid, i, desc.desc.bNumInterfaces) )
			return false;

		// add to the map
		(*this)[desc.desc.bConfigurationValue] = desc;
		dout("  config added to map with bConfigurationValue: " << (unsigned int)desc.desc.bConfigurationValue);
	}

	return m_valid = true;
}


///////////////////////////////////////////////////////////////////////////////
// DeviceDiscovery

DeviceDiscovery::DeviceDiscovery(libusb_device_id_t devid)
	: m_valid(false)
{
	Discover(devid);
}

bool DeviceDiscovery::Discover(libusb_device_id_t devid)
{
	// start fresh
	configs.clear();
	m_valid = false;

	if( libusb_get_device_desc(devid, &desc) < 0 )
		return false;
	dout("device_desc loaded");

	m_valid = configs.Discover(devid, desc.bNumConfigurations);
	return m_valid;
}

} // namespace Usb

