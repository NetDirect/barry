///
/// \file	probe.h
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
	libusb_device_id_t m_dev;
	uint32_t m_pin;
	Usb::EndpointPair m_ep;
};

std::ostream& operator<< (std::ostream &os, const ProbeResult &pr);


class Probe
{
	std::vector<ProbeResult> m_results;

	bool Parse(const Data &data, ProbeResult &result);
public:
	Probe();

	int GetCount() const { return m_results.size(); }
	const ProbeResult& Get(int index) const { return m_results[index]; }
};


} // namespace Barry

#endif

