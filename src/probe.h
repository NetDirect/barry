///
/// \file	probe.h
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

#ifndef __BARRY_PROBE_H__
#define __BARRY_PROBE_H__

#include "usbwrap.h"
#include <vector>
#include <iosfwd>
#include <stdint.h>

namespace Barry {

struct ProbeResult
{
	Usb::DeviceIDType m_dev;
	unsigned char m_interface;
	uint32_t m_pin;
	Usb::EndpointPair m_ep;
	uint8_t m_zeroSocketSequence;
	std::string m_description;
};

std::ostream& operator<< (std::ostream &os, const ProbeResult &pr);


class Probe
{
	std::vector<ProbeResult> m_results;

	std::vector<std::string> m_fail_msgs;
	int m_fail_count;

	bool CheckSize(const Data &data, unsigned int required);
	bool ParsePIN(const Data &data, ProbeResult &result);
	bool ParseDesc(const Data &data, ProbeResult &result);

protected:
	void ProbeMatching(int vendor, int product,
		const char *busname, const char *devname);
	void ProbeDevice(Usb::DeviceIDType devid);

public:
	Probe(const char *busname = 0, const char *devname = 0);

	int GetCount() const { return m_results.size(); }
	int GetFailCount() const { return m_fail_count; }

	const std::string& GetFailMsg(int index) const { return m_fail_msgs[index]; }
	const ProbeResult& Get(int index) const { return m_results[index]; }

	int FindActive(uint32_t pin = 0) const;	// returns -1 if pin not found
						// or if no devices
};


} // namespace Barry

#endif

