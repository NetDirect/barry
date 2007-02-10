///
/// \file	probe.cc
///		USB Blackberry detection routines
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "common.h"
#include "probe.h"
#include "usbwrap.h"
#include "data.h"
#include "endian.h"
#include "error.h"
#include "debug.h"
#include <iomanip>

using namespace Usb;

namespace Barry {

unsigned char Intro_Sends[][32] = {
	// packet #1
	{ 0x00, 0x00, 0x10, 0x00, 0x01, 0xff, 0x00, 0x00,
	  0xa8, 0x18, 0xda, 0x8d, 0x6c, 0x02, 0x00, 0x00 },

	// packet #2
	{ 0x00, 0x00, 0x0c, 0x00, 0x05, 0xff, 0x00, 0x01,
	  0x14, 0x00, 0x01, 0x00 },

	// packet #3
	{ 0x00, 0x00, 0x0c, 0x00, 0x05, 0xff, 0x00, 0x02,
	  0x08, 0x00, 0x04, 0x00 }
};


unsigned char Intro_Receives[][32] = {
	// response to packet #1
	{ 0x00, 0x00, 0x10, 0x00, 0x02, 0xff, 0x00, 0x00,
	  0xa8, 0x18, 0xda, 0x8d, 0x6c, 0x02, 0x00, 0x00 },

	// response to packet #2
	{ 0x00, 0x00, 0x20, 0x00, 0x06, 0xff, 0x00, 0x01,
	  0x14, 0x00, 0x01, 0x00, 0x51, 0xe1, 0x33, 0x6b,
	  0xf3, 0x09, 0xbc, 0x37, 0x3b, 0xa3, 0x5e, 0xed,
	  0xff, 0x30, 0xa1, 0x3a, 0x60, 0xc9, 0x81, 0x8e },

	{ 0x00, 0x00, 0x14, 0x00, 0x06, 0xff, 0x00, 0x02,
	  0x08, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
	  0xe3, 0xef, 0x09, 0x30 }
};

namespace {

	unsigned int GetSize(const unsigned char *packet)
	{
		uint16_t size = *((uint16_t *)&packet[2]);
		return btohs(size);
	}

	bool Intro(int IntroIndex, const EndpointPair &ep, Device &dev, Data &response)
	{
		dev.BulkWrite(ep.write, Intro_Sends[IntroIndex],
			GetSize(Intro_Sends[IntroIndex]));
		dev.BulkRead(ep.read, response);
		ddout("BulkRead (" << (unsigned int)ep.read << "):\n" << response);
		return true;
	}

} // anonymous namespace


bool Probe::Parse(const Data &data, ProbeResult &result)
{
	// validate response data
	const unsigned char *pd = data.GetData();

	if( GetSize(pd) != (unsigned int) data.GetSize() ||
	    data.GetSize() < 0x14 ||
	    pd[4] != 0x06 )
	{
		return false;
//		throw Error("Probe: Unexpected reponse data in parse");
	}

	// capture the PIN
	result.m_pin = btohl(*((uint32_t *) &pd[16]));
        
	return true;
}

Probe::Probe()
{
	Usb::DeviceIDType devid;

	// Search for standard product ID first
	{
		Match match(VENDOR_RIM, PRODUCT_RIM_BLACKBERRY);
		while( match.next_device(&devid) )
			ProbeDevice(devid);
	}

	// Search for Pearl devices second
	{
		// FIXME - the actual probing code doesn't work on
		// productID 6 devices... we need a capture from
		// someone who has such a device
		Match match(VENDOR_RIM, PRODUCT_RIM_PEARL);
		while( match.next_device(&devid) )
			ProbeDevice(devid);
	}
}

void Probe::ProbeDevice(Usb::DeviceIDType devid)
{
	// skip if we can't properly discover device config
	DeviceDiscovery discover(devid);
	EndpointDiscovery &ed = discover.configs[BLACKBERRY_CONFIGURATION]
		.interfaces[BLACKBERRY_INTERFACE]
		.endpoints;
	if( !ed.IsValid() || ed.GetEndpointPairs().size() == 0 )
		return;


	ProbeResult result;
	result.m_dev = devid;

	// find the first bulk read/write endpoint pair that answers
	// to our probe commands
	// Search in reverse, since evidence indicates the last pairs
	// are the ones we need.
	for(size_t i = ed.GetEndpointPairs().size(); i; i-- ) {
		const EndpointPair &ep = ed.GetEndpointPairs()[i-1];
		if( ep.type == USB_ENDPOINT_TYPE_BULK ) {
			result.m_ep = ep;

			Device dev(devid);
//				dev.Reset();
//				sleep(5);

			if( !dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
				throw Error(dev.GetLastError(),
					"Probe: SetConfiguration failed");

			Interface iface(dev, BLACKBERRY_INTERFACE);

			Data data;
			dev.BulkDrain(ep.read);
			if( Intro(0, ep, dev, data) && Intro(1, ep, dev, data) &&
			    Intro(2, ep, dev, data) && Parse(data, result) )
			{
				// all info obtained, add to list
				m_results.push_back(result);
				ddout("Using ReadEndpoint: " << (unsigned int)result.m_ep.read);
				ddout("      WriteEndpoint: " << (unsigned int)result.m_ep.write);
				break;
			}
		}
	}

	if( !result.m_ep.IsComplete() )
		ddout("Unable to discover endpoint pair for one device.");
}

int Probe::FindActive(uint32_t pin) const
{
	for( int i = 0; i < GetCount(); i++ ) {
		if( Get(i).m_pin == pin )
			return i;
	}
	if( pin == 0 ) {
		// can we default to a single device?
		if( GetCount() == 1 )
			return 0;	// yes!
	}

	// PIN not found
	return -1;
}

std::ostream& operator<< (std::ostream &os, const ProbeResult &pr)
{
	os << "Device ID: " << pr.m_dev << std::setbase(16) << ". PIN: "
		<< pr.m_pin;
	return os;
}

} // namespace Barry

