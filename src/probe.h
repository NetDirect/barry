///
/// \file	probe.h
///		USB Blackberry detection routines
///

/*
    Copyright (C) 2005-2006, Net Direct Inc. (http://www.netdirect.ca/)

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

// forward declarations
class Data;


namespace Barry {

struct ProbeResult
{
	Usb::DeviceIDType m_dev;
	uint32_t m_pin;
	Usb::EndpointPair m_ep;
};

std::ostream& operator<< (std::ostream &os, const ProbeResult &pr);


class Probe
{
	std::vector<ProbeResult> m_results;

	bool Parse(const Data &data, ProbeResult &result);

protected:
	void ProbeDevice(Usb::DeviceIDType devid);

public:
	Probe();

	int GetCount() const { return m_results.size(); }
	const ProbeResult& Get(int index) const { return m_results[index]; }

	int FindActive(uint32_t pin = 0) const;	// returns -1 if pin not found
						// or if no devices
};


} // namespace Barry

#endif

