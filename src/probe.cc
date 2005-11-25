///
/// \file	probe.cc
///		USB Blackberry detection routines
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "error.h"
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
		return *((uint16_t *)&packet[2]);
	}

	void Intro(int IntroIndex, Device &dev, Data &response)
	{
		IO rd = dev.ABulkRead(READ_ENDPOINT, response);
		IO wr = dev.ABulkWrite(WRITE_ENDPOINT, Intro_Sends[IntroIndex],
			GetSize(Intro_Sends[IntroIndex]));

		rd.Wait();
		wr.Wait();

		response.ReleaseBuffer(rd.GetStatus() >= 0 ? rd.GetSize() : 0);
	}

} // anonymous namespace

void Probe::Parse(const Data &data, ProbeResult &result)
{
	// validate response data
	const unsigned char *pd = data.GetData();
	if( GetSize(pd) != (unsigned int) data.GetSize() ||
	    data.GetSize() < 0x14 ||
	    pd[4] != 0x06 )
	{
		throw BError("Probe: Unexpected reponse data in parse");
	}

	// capture the PIN
	result.m_pin = *((uint32_t *) &pd[16]);
}

Probe::Probe()
{
	Match match(VENDOR_RIM, PRODUCT_RIM_BLACKBERRY);

	libusb_device_id_t devid;
	while( match.next_device(&devid) ) {
		ProbeResult result;
		result.m_dev = devid;

		Device dev(devid);
		dev.Reset();
		sleep(5);

		Interface iface(dev, BLACKBERRY_INTERFACE);

		if( !dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
			throw BError(dev.GetLastError(),
				"Probe: SetConfiguration failed");

		Data data;
		Intro(0, dev, data);
		Intro(1, dev, data);
		Intro(2, dev, data);
		Parse(data, result);

		// all info obtained, add to list
		m_results.push_back(result);
	}
}

std::ostream& operator<< (std::ostream &os, const ProbeResult &pr)
{
	os << "Device ID: " << pr.m_dev << std::setbase(16) << ". PIN: "
		<< pr.m_pin;
	return os;
}

} // namespace Barry

