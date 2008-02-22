///
/// \file	probe.cc
///		USB Blackberry detection routines
///

/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <errno.h>
#include <string.h>

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
		try {
			dev.BulkRead(ep.read, response, 500);
		}
		catch( Usb::Timeout &to ) {
			ddout("BulkRead: " << to.what());
			return false;
		}
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

bool Probe::ParsePIN(const Data &data, uint32_t &pin)
{
	// validate response data
	const unsigned char *pd = data.GetData();

	if( !CheckSize(data, 0x14) )
		return false;

	// capture the PIN
	pin = btohl(*((uint32_t *) &pd[16]));

	return true;
}

bool Probe::ParseDesc(const Data &data, std::string &desc)
{
	if( !CheckSize(data, 29) )
		return false;

	// capture the description
	const char *d = (const char*) &data.GetData()[28];
	int maxlen = data.GetSize() - 28;
	desc.assign(d, strnlen(d, maxlen));

	return true;
}

Probe::Probe(const char *busname, const char *devname)
	: m_fail_count(0)
{
	// let the programmer pass in "" as well as 0
	if( busname && !strlen(busname) )
		busname = 0;
	if( devname && !strlen(devname) )
		devname = 0;

	// Search for standard product ID first
	ProbeMatching(VENDOR_RIM, PRODUCT_RIM_BLACKBERRY, busname, devname);

	// Search for Pearl devices second
	//
	// productID 6 devices (PRODUCT_RIM_PEARL) do not expose
	// the USB class 255 interface we need, but only the
	// Mass Storage one.  Here we search for PRODUCT_RIM_PEARL_DUAL,
	// (ID 4) which has both enabled.
	ProbeMatching(VENDOR_RIM, PRODUCT_RIM_PEARL_DUAL, busname, devname);
}

void Probe::ProbeMatching(int vendor, int product,
			const char *busname, const char *devname)
{
	Usb::DeviceIDType devid;

	Match match(vendor, product, busname, devname);
	while( match.next_device(&devid) ) try {
		ProbeDevice(devid);
	}
	catch( Usb::Error &e ) {
		dout("Usb::Error exception caught: " << e.what());
		if( e.libusb_errcode() == -EBUSY ) {
			m_fail_count++;
			m_fail_msgs.push_back(e.what());
		}
		else {
			throw;
		}
	}
}

void Probe::ProbeDevice(Usb::DeviceIDType devid)
{
	// skip if we can't properly discover device config
	DeviceDiscovery discover(devid);
	ConfigDesc &config = discover.configs[BLACKBERRY_CONFIGURATION];

	// search for interface class
	InterfaceDiscovery::base_type::iterator idi = config.interfaces.begin();
	for( ; idi != config.interfaces.end(); idi++ ) {
		if( idi->second.desc.bInterfaceClass == BLACKBERRY_DB_CLASS )
			break;
	}
	if( idi == config.interfaces.end() ) {
		dout("Probe: Interface with BLACKBERRY_DB_CLASS ("
			<< BLACKBERRY_DB_CLASS << ") not found.");
		return;	// not found
	}

	unsigned char InterfaceNumber = idi->second.desc.bInterfaceNumber;
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

	// open device
	Device dev(devid);
//	dev.Reset();
//	sleep(5);

	//  make sure we're talking to the right config
	unsigned char cfg;
	if( !dev.GetConfiguration(cfg) )
		throw Usb::Error(dev.GetLastError(),
			"Probe: GetConfiguration failed");
	if( cfg != BLACKBERRY_CONFIGURATION ) {
		if( !dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
			throw Usb::Error(dev.GetLastError(),
				"Probe: SetConfiguration failed");
	}

	// open interface
	Interface iface(dev, InterfaceNumber);

	// find the first bulk read/write endpoint pair that answers
	// to our probe commands
	// Start with second pair, since evidence indicates the later pairs
	// are the ones we need.
	size_t i;
	for(i = ed.GetEndpointPairs().size() > 1 ? 1 : 0;
	    i < ed.GetEndpointPairs().size();
	    i++ )
	{
		const EndpointPair &ep = ed.GetEndpointPairs()[i];
		if( ep.type == USB_ENDPOINT_TYPE_BULK ) {

			uint32_t pin;
			uint8_t zeroSocketSequence;
			std::string desc;
			if( ProbePair(dev, ep, pin, desc, zeroSocketSequence) ) {
				result.m_ep = ep;
				result.m_description = desc;
				result.m_zeroSocketSequence = zeroSocketSequence;
				break;
			}
		}
		else {
			dout("Probe: Skipping non-bulk endpoint pair (offset: "
				<< i-1 << ") ");
		}
	}

	// check for ip modem endpoints
	i++;
	if( i < ed.GetEndpointPairs().size() ) {
		const EndpointPair &ep = ed.GetEndpointPairs()[i];
		if( ProbeModem(dev, ep) ) {
			result.m_epModem = ep;
		}
	}

	// add to list
	if( result.m_ep.IsComplete() ) {
		m_results.push_back(result);
		ddout("Using ReadEndpoint: " << (unsigned int)result.m_ep.read);
		ddout("      WriteEndpoint: " << (unsigned int)result.m_ep.write);
	}
	else {
		ddout("Unable to discover endpoint pair for one device.");
	}
}

bool Probe::ProbePair(Usb::Device &dev,
			const Usb::EndpointPair &ep,
			uint32_t &pin,
			std::string &desc,
			uint8_t &zeroSocketSequence)
{
	dev.ClearHalt(ep.read);
	dev.ClearHalt(ep.write);

	Data data;
	dev.BulkDrain(ep.read);
	if( !Intro(0, ep, dev, data) ) {
		dout("Probe: Intro(0) failed");
		return false;
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
	    !ParsePIN(receive, pin) )
	{
		dout("Probe: unable to fetch PIN");
		return false;
	}

	// fetch Description
	packet.GetAttribute(SB_OBJECT_PROFILE, SB_ATTR_PROFILE_DESC);
	socket.Send(packet);
	// response ObjectID does not match request... :-/
	if( // packet.ObjectID() != SB_OBJECT_PROFILE ||
	    packet.AttributeID() != SB_ATTR_PROFILE_DESC ||
	    !ParseDesc(receive, desc) )
	{
		dout("Probe: unable to fetch description");
	}

	// more unknowns:
	for( uint16_t attr = 5; attr < 9; attr++ ) {
		packet.GetAttribute(SB_OBJECT_SOCKET_UNKNOWN, attr);
		socket.Send(packet);
		// FIXME parse these responses, if they turn
		// out to be important
	}

	// all info obtained!
	zeroSocketSequence = socket.GetZeroSocketSequence();
	return true;
}

// Thanks to Jason Scott (bb_usb.c) for reverse engineering this
bool Probe::ProbeModem(Usb::Device &dev, const Usb::EndpointPair &ep)
{
	int num_read;
	char data[255];
	int local_errno;

	num_read = usb_control_msg(dev.GetHandle(),
		/* bmRequestType */ USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		/* bRequest */ 0xa5,
		/* wValue */ 0,
		/* wIndex */ 1,
		/* data */ data,
		/* wLength */ sizeof(data),
		/* timeout */ 2000);
	local_errno = errno;
	if( num_read > 1 ) {
		if( data[0] == 0x02 ) {
			return true;
		}
	}
	return false;
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

void ProbeResult::DumpAll(std::ostream &os) const
{
	os << *this
	   << ", Interface: 0x" << std::hex << (unsigned int) m_interface
	   << ", Endpoints: (read: 0x" << std::hex << (unsigned int) m_ep.read
		<< ", write: 0x" << std::hex << (unsigned int) m_ep.write
		<< ", type: 0x" << std::hex << (unsigned int) m_ep.type
	   << ", ZeroSocketSequence: 0x" << std::hex << (unsigned int) m_zeroSocketSequence;
}

std::ostream& operator<< (std::ostream &os, const ProbeResult &pr)
{
	os << "Device ID: " << pr.m_dev
	   << std::hex << ". PIN: " << pr.m_pin
	   << ", Description: " << pr.m_description;
	return os;
}

} // namespace Barry

