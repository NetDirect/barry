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
#include "packet.h"
#include "socket.h"
#include "protocol.h"
#include "record-internal.h"
#include "strnlen.h"
#include <iomanip>

using namespace Usb;

namespace Barry {

unsigned char Intro_Sends[][32] = {
	// packet #1
	{ 0x00, 0x00, 0x10, 0x00, 0x01, 0xff, 0x00, 0x00,
	  0xa8, 0x18, 0xda, 0x8d, 0x6c, 0x02, 0x00, 0x00 }
};


unsigned char Intro_Receives[][32] = {
	// response to packet #1
	{ 0x00, 0x00, 0x10, 0x00, 0x02, 0xff, 0x00, 0x00,
	  0xa8, 0x18, 0xda, 0x8d, 0x6c, 0x02, 0x00, 0x00 }
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


bool Probe::CheckSize(const Data &data, unsigned int required)
{
	const unsigned char *pd = data.GetData();

	if( GetSize(pd) != (unsigned int) data.GetSize() ||
	    data.GetSize() < required ||
	    pd[4] != SB_COMMAND_FETCHED_ATTRIBUTE )
	{
		dout("Probe: Parse data failure: GetSize(pd): " << GetSize(pd)
			<< ", data.GetSize(): " << data.GetSize()
			<< ", pd[4]: " << (unsigned int) pd[4]);
		return false;
	}

	return true;
}

bool Probe::ParsePIN(const Data &data, ProbeResult &result)
{
	// validate response data
	const unsigned char *pd = data.GetData();

	if( !CheckSize(data, 0x14) )
		return false;

	// capture the PIN
	result.m_pin = btohl(*((uint32_t *) &pd[16]));

	return true;
}

bool Probe::ParseDesc(const Data &data, ProbeResult &result)
{
	if( !CheckSize(data, 29) )
		return false;

	// capture the description
	const char *desc = (const char*) &data.GetData()[28];
	int maxlen = data.GetSize() - 28;
	result.m_description.assign(desc, strnlen(desc, maxlen));

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
		// productID 6 devices (PRODUCT_RIM_PEARL) do not expose
		// the USB class 255 interface we need, but only the
		// Mass Storage one.  Here we search for PRODUCT_RIM_PEARL_DUAL,
		// (ID 4) which has both enabled.
		Match match(VENDOR_RIM, PRODUCT_RIM_PEARL_DUAL);
		while( match.next_device(&devid) )
			ProbeDevice(devid);
	}
}

void Probe::ProbeDevice(Usb::DeviceIDType devid)
{
	// skip if we can't properly discover device config
	DeviceDiscovery discover(devid);
	ConfigDesc &config = discover.configs[BLACKBERRY_CONFIGURATION];

	// search for interface class
	InterfaceDiscovery::base_type::iterator i = config.interfaces.begin();
	for( ; i != config.interfaces.end(); i++ ) {
		if( i->second.desc.bInterfaceClass == BLACKBERRY_DB_CLASS )
			break;
	}
	if( i == config.interfaces.end() ) {
		dout("Probe: Interface with BLACKBERRY_DB_CLASS ("
			<< BLACKBERRY_DB_CLASS << ") not found.");
		return;	// not found
	}

	unsigned char InterfaceNumber = i->second.desc.bInterfaceNumber;
	dout("Probe: using InterfaceNumber: " << (unsigned int) InterfaceNumber);

	// check endpoint validity
	EndpointDiscovery &ed = config.interfaces[InterfaceNumber].endpoints;
	if( !ed.IsValid() || ed.GetEndpointPairs().size() == 0 ) {
		dout("Probe: endpoint invalid.   ed.IsValud() == "
			<< (ed.IsValid() ? "true" : "false")
			<< ", ed.GetEndpointPairs().size() == "
			<< ed.GetEndpointPairs().size());
		return;
	}

	ProbeResult result;
	result.m_dev = devid;
	result.m_interface = InterfaceNumber;
	result.m_zeroSocketSequence = 0;

	// find the first bulk read/write endpoint pair that answers
	// to our probe commands
	// Start with second pair, since evidence indicates the later pairs
	// are the ones we need.
	for(size_t i = ed.GetEndpointPairs().size() > 1 ? 1 : 0;
	    i < ed.GetEndpointPairs().size();
	    i++ )
	{
		const EndpointPair &ep = ed.GetEndpointPairs()[i];
		if( ep.type == USB_ENDPOINT_TYPE_BULK ) {
			result.m_ep = ep;

			Device dev(devid);
//				dev.Reset();
//				sleep(5);

			if( !dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
				throw Usb::Error(dev.GetLastError(),
					"Probe: SetConfiguration failed");

			Interface iface(dev, InterfaceNumber);

			Data data;
			dev.BulkDrain(ep.read);
			if( !Intro(0, ep, dev, data) ) {
				dout("Probe: Intro(0) failed");
				continue;
			}

			SocketZero socket(dev, ep.write, ep.read);

			Data send, receive;
			ZeroPacket packet(send, receive);

			// unknown attribute: 0x14 / 0x01
			packet.GetAttribute(SB_OBJECT_INITIAL_UNKNOWN,
				SB_ATTR_INITIAL_UNKNOWN);
			socket.Send(packet);

			// fetch PIN
			packet.GetAttribute(SB_OBJECT_PROFILE, SB_ATTR_PROFILE_PIN);
			socket.Send(packet);
			if( packet.ObjectID() != SB_OBJECT_PROFILE ||
			    packet.AttributeID() != SB_ATTR_PROFILE_PIN ||
			    !ParsePIN(receive, result) )
			{
				dout("Probe: unable to fetch PIN");
				continue;
			}

			// fetch Description
			packet.GetAttribute(SB_OBJECT_PROFILE, SB_ATTR_PROFILE_DESC);
			socket.Send(packet);
			// response ObjectID does not match request... :-/
			if( // packet.ObjectID() != SB_OBJECT_PROFILE ||
			    packet.AttributeID() != SB_ATTR_PROFILE_DESC ||
			    !ParseDesc(receive, result) )
			{
				dout("Probe: unable to fetch description");
				// this is a relatively new feature, so don't
				// fail here... just blank the description
				result.m_description.clear();
			}

			// more unknowns:
			for( uint16_t attr = 5; attr < 9; attr++ ) {
				packet.GetAttribute(SB_OBJECT_SOCKET_UNKNOWN, attr);
				socket.Send(packet);
				// FIXME parse these responses, if they turn
				// out to be important
			}


			// all info obtained, add to list
			result.m_zeroSocketSequence = socket.GetZeroSocketSequence();
			m_results.push_back(result);
			ddout("Using ReadEndpoint: " << (unsigned int)result.m_ep.read);
			ddout("      WriteEndpoint: " << (unsigned int)result.m_ep.write);
			break;
		}
		else {
			dout("Probe: Skipping non-bulk endpoint pair (offset: "
				<< i-1 << ") ");
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
	os << "Device ID: " << pr.m_dev
	   << std::hex << ". PIN: " << pr.m_pin
	   << ", Description: " << pr.m_description;
	return os;
}

} // namespace Barry

