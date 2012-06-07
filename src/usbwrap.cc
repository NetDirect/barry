///
/// \file	usbwrap.cc
///		USB API wrapper
///

/*
    Copyright (C) 2005-2012, Chris Frey
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


#include "usbwrap.h"
#include "data.h"
#include "error.h"
#include "config.h"
#include "debug.h"

#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#ifndef __DEBUG_MODE__
#define __DEBUG_MODE__
#endif
#include "debug.h"

// Pull in the correct Usb::LibraryInterface
#if defined USE_LIBUSB_0_1
#include "usbwrap_libusb.h"
#elif defined USE_LIBUSB_1_0
#include "usbwrap_libusb_1_0.h"
#else
#error No usb library interface selected.
#endif


namespace Usb {

///////////////////////////////////////////////////////////////////////////////
// Usb::Error exception class

static std::string GetErrorString(int libusb_errcode, const std::string &str)
{
	std::ostringstream oss;
	oss << "(";

	if( libusb_errcode ) {
		oss << std::dec << libusb_errcode << " ("
		    << LibraryInterface::TranslateErrcode(libusb_errcode)
		    << "), ";
	}
	oss << LibraryInterface::GetLastErrorString(libusb_errcode) << "): ";
	oss << str;
	return oss.str();
}

Error::Error(const std::string &str)
	: Barry::Error(str)
	, m_libusb_errcode(0)
{
}

Error::Error(int libusb_errcode, const std::string &str)
	: Barry::Error(libusb_errcode == 0 ? str : GetErrorString(libusb_errcode, str))
	, m_libusb_errcode(libusb_errcode)
{
}

int Error::system_errcode() const
{
	return LibraryInterface::TranslateErrcode(m_libusb_errcode);
}

///////////////////////////////////////////////////////////////////////////////
// Usb::Timeout exception class

Timeout::Timeout(const std::string &str)
	: Error(str)
{
}

Timeout::Timeout(int errcode, const std::string &str)
	: Error(errcode, str)
{
}

///////////////////////////////////////////////////////////////////////////////
// EndpointPair

EndpointPair::EndpointPair()
	: read(0), write(0), type(EndpointDescriptor::InvalidType)
{
}

bool EndpointPair::IsTypeSet() const
{
	return type != EndpointDescriptor::InvalidType;
}

bool EndpointPair::IsComplete() const
{
	return read && write && IsTypeSet();
}

bool EndpointPair::IsBulk() const
{
	return type == EndpointDescriptor::BulkType;
}


///////////////////////////////////////////////////////////////////////////////
// EndpointPairings

EndpointPairings::EndpointPairings(const std::vector<EndpointDescriptor*>& eps)
	: m_valid(false)
{
	// parse the endpoint into read/write sets, if possible,
	// going in discovery order...
	// Assumptions:
	//	- endpoints of related utility will be grouped
	//	- endpoints with same type will be grouped
	//	- endpoints that do not meet the above assumptions
	//		do not belong in a pair
	EndpointPair pair;

	if( eps.size() == 0 ) {
		dout("EndpointPairing:: empty interface pointer");
		return;
	}

	std::vector<EndpointDescriptor*>::const_iterator iter = eps.begin();
	while( iter != eps.end() ) {
		const EndpointDescriptor& desc = **iter;
		if( desc.IsRead() ) {
			// Read endpoint
			pair.read = desc.GetAddress();
			dout("        pair.read = 0x" << std::hex << (unsigned int)pair.read);
			if( pair.IsTypeSet() && pair.type != desc.GetType() ) {
				// if type is already set, we must start over
				pair.write = 0;
			}
		} else {
			// Write endpoint
			pair.write = desc.GetAddress();
			dout("        pair.write = 0x" << std::hex << (unsigned int)pair.write);
			if( pair.IsTypeSet() && pair.type != desc.GetType() ) {
				// if type is already set, we must start over
				pair.read = 0;
			}
		}
		pair.type = desc.GetType();

		dout("        pair.type = 0x" << std::hex << (unsigned int)pair.type);

		// if pair is complete, add to array
		if( pair.IsComplete() ) {
			push_back(pair);

			dout("        pair added! ("
			     << "read: 0x" << std::hex << (unsigned int)pair.read << ","
			     << "write: 0x" << std::hex << (unsigned int)pair.write << ","
			     << "type: 0x" << std::hex << (unsigned int)pair.type << ")");
			pair = EndpointPair();	// clear
		}
		++iter;
	}

	m_valid = true;
}

EndpointPairings::~EndpointPairings()
{
}

bool EndpointPairings::IsValid() const
{
	return m_valid;
}

///////////////////////////////////////////////////////////////////////////////
// EndpointDescriptor

bool EndpointDescriptor::IsRead() const
{
	return m_read;
}

uint8_t EndpointDescriptor::GetAddress() const
{
	return m_addr;
}

EndpointDescriptor::EpType EndpointDescriptor::GetType() const
{
	return m_type;
}

///////////////////////////////////////////////////////////////////////////////
// Match

Match::Match(DeviceList& devices,
	     int vendor, int product,
	     const char *busname, const char *devname)
	: m_list(devices.MatchDevices(vendor, product, busname, devname))
	, m_iter(m_list.begin())
{

}

Match::~Match()
{
}

bool Match::next_device(Usb::DeviceID& devid)
{
	if( m_iter != m_list.end() ) {
		devid = *m_iter;
		++m_iter;
		return true;
	}
	return false;
}

} // namespace Usb

