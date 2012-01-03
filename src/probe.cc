///
/// \file	probe.cc
///		USB Blackberry detection routines
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "protostructs.h"
#include "record-internal.h"
#include "strnlen.h"
#include "configfile.h"
#include "platform.h"
#include <iomanip>
#include <sstream>
#include <errno.h>
#include <string.h>
#include "ios_state.h"

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
		const Protocol::Packet *pack = (const Protocol::Packet*) packet;
		return btohs(pack->size);
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
	memcpy(&pin, &pd[16], sizeof(pin));
	pin = btohl(pin);

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

Probe::Probe(const char *busname, const char *devname,
		const Usb::EndpointPair *epp)
	: m_fail_count(0)
	, m_epp_override(epp)
{
	if( m_epp_override ) {
		m_epp = *epp;
	}

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
	// And a special case, which behaves similar to the PEARL_DUAL,
	// but with a unique Product ID.
	ProbeMatching(VENDOR_RIM, PRODUCT_RIM_PEARL_8120, busname, devname);
	// And one more!  The Pearl Flip
	ProbeMatching(VENDOR_RIM, PRODUCT_RIM_PEARL_FLIP, busname, devname);

	// And one more time, for the Blackberry Storm
	ProbeMatching(VENDOR_RIM, PRODUCT_RIM_STORM, busname, devname);
}

void Probe::ProbeMatching(int vendor, int product,
			const char *busname, const char *devname)
{
	Usb::DeviceID devid;

	Match match(m_devices, vendor, product, busname, devname);
	while( match.next_device(devid) ) try {
		ProbeDevice(devid);
	}
	catch( Usb::Error &e ) {
		dout("Usb::Error exception caught: " << e.what());
		if( e.system_errcode() == -EBUSY ) {
			m_fail_count++;
			m_fail_msgs.push_back(e.what());
		}
		else {
			throw;
		}
	}
}

void Probe::ProbeDevice(Usb::DeviceID& devid)
{
	// skip if we can't properly discover device config
	DeviceDescriptor desc(devid);
	ConfigDescriptor* config = desc[BLACKBERRY_CONFIGURATION];
	if( !config ) {
		dout("Probe: No device descriptor for BlackBerry config (config id: "
			<< BLACKBERRY_CONFIGURATION << ")");
		return;	// not found
	}

	// search for interface class
	ConfigDescriptor::base_type::iterator idi = config->begin();
	for( ; idi != config->end(); idi++ ) {
		if( idi->second->GetClass() == BLACKBERRY_DB_CLASS )
			break;
	}
	if( idi == config->end() ) {
		dout("Probe: Interface with BLACKBERRY_DB_CLASS ("
			<< BLACKBERRY_DB_CLASS << ") not found.");
		return;	// not found
	}

	unsigned char InterfaceNumber = idi->second->GetNumber();
	unsigned char InterfaceAltSetting = idi->second->GetAltSetting();
	dout("Probe: using InterfaceNumber: " << (unsigned int) InterfaceNumber <<
	     " AltSetting: " << (unsigned int) InterfaceAltSetting);

	// check endpoint validity
	EndpointPairings ep(*(*config)[InterfaceNumber]);
	if( !ep.IsValid() || ep.size() == 0 ) {
		dout("Probe: endpoint invalid.   ep.IsValid() == "
			<< (ep.IsValid() ? "true" : "false")
			<< ", ep.size() == "
			<< ep.size());
		return;
	}

	ProbeResult result;
	result.m_dev = devid;
	result.m_interface = InterfaceNumber;
	result.m_altsetting = InterfaceAltSetting;
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
	if( cfg != BLACKBERRY_CONFIGURATION || MUST_SET_CONFIGURATION ) {
		if( !dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
			throw Usb::Error(dev.GetLastError(),
				"Probe: SetConfiguration failed");
	}

	// open interface
	Interface iface(dev, InterfaceNumber);

	// Try the initial probing of endpoints
	ProbeDeviceEndpoints(dev, ep, result);

	if( !result.m_ep.IsComplete() ) {
		// Probing of end-points failed, so try reprobing
		// after calling usb_set_altinterface().
		//
		// Calling usb_set_altinterface() should be harmless
		// and can help the host and device to synchronize the
		// USB state, especially on FreeBSD and Mac OS X.
		// However it can cause usb-storage URBs to be lost
		// on some devices, so is only used if necessary.
		dout("Probe: probing endpoints failed, retrying after setting alternate interface");
		
		iface.SetAltInterface(InterfaceAltSetting);
		result.m_needSetAltInterface = true;
		ProbeDeviceEndpoints(dev, ep, result);
	}

	// add to list
	if( result.m_ep.IsComplete() ) {
		// before adding to list, try to load the device's
		// friendly name from the configfile... but don't
		// fail if we can't do it
		try {
			ConfigFile cfg(result.m_pin);
			result.m_cfgDeviceName = cfg.GetDeviceName();
		}
		catch( Barry::ConfigFileError & ) {
			// ignore...
		}

		m_results.push_back(result);
		ddout("Using ReadEndpoint: " << (unsigned int)result.m_ep.read);
		ddout("      WriteEndpoint: " << (unsigned int)result.m_ep.write);
	}
	else {
		ddout("Unable to discover endpoint pair for one device.");
	}
}

void Probe::ProbeDeviceEndpoints(Device &dev, EndpointPairings &ed, ProbeResult &result)
{
	if( m_epp_override ) {
		// user has given us endpoints to try... so try them
		uint32_t pin;
		uint8_t zeroSocketSequence;
		std::string desc;
		bool needClearHalt;
		if( ProbePair(dev, m_epp, pin, desc, zeroSocketSequence, needClearHalt) ) {
			// looks good, finish filling out the result
			result.m_ep = m_epp;
			result.m_pin = pin;
			result.m_description = desc;
			result.m_zeroSocketSequence = zeroSocketSequence;
			result.m_needClearHalt = needClearHalt;
		}
	}
	else {
		// find the first bulk read/write endpoint pair that answers
		// to our probe commands
		// Start with second pair, since evidence indicates the later pairs
		// are the ones we need.
		size_t i;
		for(i = ed.size() > 1 ? 1 : 0;
		    i < ed.size();
		    i++ )
		{
			const EndpointPair &ep = ed[i];
			if( ep.type == Usb::EndpointDescriptor::BulkType ) {

				uint32_t pin;
				uint8_t zeroSocketSequence;
				std::string desc;
				bool needClearHalt;
				if( ProbePair(dev, ep, pin, desc, zeroSocketSequence, needClearHalt) ) {
					result.m_ep = ep;
					result.m_pin = pin;
					result.m_description = desc;
					result.m_zeroSocketSequence = zeroSocketSequence;
					result.m_needClearHalt = needClearHalt;
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
		if( i < ed.size() ) {
			const EndpointPair &ep = ed[i];
			if( ProbeModem(dev, ep) ) {
				result.m_epModem = ep;
			}
		}
	}
}

bool Probe::ProbePair(Usb::Device &dev,
			const Usb::EndpointPair &ep,
			uint32_t &pin,
			std::string &desc,
			uint8_t &zeroSocketSequence,
			bool &needClearHalt)
{
	// Initially assume that clear halt isn't needed as it causes some
	// devices to drop packets. The suspicion is that the toggle bits
	// get out of sync, but this hasn't been confirmed with hardware
	// tracing.
	//
	// It is possible to always use clear halt, as long as SET
	// INTERFACE has been sent before, via usb_set_altinterface().
	// However this has the side affect that any outstanding URBs
	// on other interfaces (i.e. usb-storage) timeout and lose
	// their data. This is not a good thing as it can corrupt the
	// file system exposed over usb-storage. This also has the
	// side-affect that usb-storage issues a port reset after the
	// 30 second timeout, which kills any current Barry
	// connection.
	//
	// To further complicate matters some devices, such as the
	// 8830, always need clear halt before they will respond to
	// probes.
	//
	// So to work with all these device quirks the probe is first
	// attempted without calling clear halt. If that probe fails
	// then a clear halt is issued followed by a retry on the
	// probing.
	needClearHalt = false;

	Data data;
	dev.BulkDrain(ep.read);
	if( !Intro(0, ep, dev, data) ) {
		// Try clearing halt and then reprobing
		dout("Probe: Intro(0) failed, retrying after clearing halt");
		dev.ClearHalt(ep.read);
		dev.ClearHalt(ep.write);
		needClearHalt = true;
		// Retry
		dev.BulkDrain(ep.read);
		if( !Intro(0, ep, dev, data) ) {
			// Still no response so fail the probe
			dout("Probe: Intro(0) still failed after clearing halt");
			return false;
		}
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

bool Probe::ProbeModem(Usb::Device &dev, const Usb::EndpointPair &ep)
{
	//
	// This check is not needed for all devices.  Some devices,
	// like the 8700 have both the RIM_UsbSerData mode and IpModem mode.
	//
	// If this function is called, then we have extra endpoints,
	// so might as well try them.
	//
	// FIXME - someday, we might wish to confirm that the endpoints
	// work as a modem, and return true/false based on that test.
	//
	return true;


// Thanks to Rick Scott (XmBlackBerry:bb_usb.c) for reverse engineering this
//	int num_read;
//	char data[255];
//	int local_errno;
//
//	num_read = usb_control_msg(dev.GetHandle(),
//		/* bmRequestType */ USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
//		/* bRequest */ 0xa5,
//		/* wValue */ 0,
//		/* wIndex */ 1,
//		/* data */ data,
//		/* wLength */ sizeof(data),
//		/* timeout */ 2000);
//	local_errno = errno;
//	if( num_read > 1 ) {
//		if( data[0] == 0x02 ) {
//			return true;
//		}
//	}
//	return false;
}

int Probe::FindActive(Barry::Pin pin) const
{
	return FindActive(m_results, pin);
}

int Probe::FindActive(const Barry::Probe::Results &results, Barry::Pin pin)
{
	int i = Find(results, pin);

	if( i == -1 && pin == 0 ) {
		// can we default to a single device?
		if( results.size() == 1 )
			return 0;	// yes!
	}

	return i;
}

int Probe::Find(const Results &results, Barry::Pin pin)
{
	Barry::Probe::Results::const_iterator ci = results.begin();
	for( int i = 0; ci != results.end(); i++, ++ci ) {
		if( ci->m_pin == pin )
			return i;
	}
	// PIN not found
	return -1;
}

void ProbeResult::DumpAll(std::ostream &os) const
{
	ios_format_state state(os);

	os << *this
	   << ", Interface: 0x" << std::hex << (unsigned int) m_interface
	   << ", Endpoints: (read: 0x" << std::hex << (unsigned int) m_ep.read
		<< ", write: 0x" << std::hex << (unsigned int) m_ep.write
		<< ", type: 0x" << std::hex << (unsigned int) m_ep.type
	   << ", ZeroSocketSequence: 0x" << std::hex << (unsigned int) m_zeroSocketSequence;
}

std::string ProbeResult::GetDisplayName() const
{
	std::ostringstream oss;
	oss << m_pin.Str();
	if( m_cfgDeviceName.size() )
		oss << " (" << m_cfgDeviceName << ")";
	return oss.str();
}

std::ostream& operator<< (std::ostream &os, const ProbeResult &pr)
{
	ios_format_state state(os);

	os << "Device ID: " << pr.m_dev.m_impl.get()
	   << ". PIN: " << pr.m_pin.Str()
	   << ", Description: " << pr.m_description;
	if( pr.m_cfgDeviceName.size() )
		os << ", Name: " << pr.m_cfgDeviceName;
	return os;
}

} // namespace Barry

