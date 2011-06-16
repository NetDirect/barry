///
/// \file	probe.h
///		USB Blackberry detection routines
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "dll.h"
#include "usbwrap.h"
#include "pin.h"
#include <vector>
#include <iosfwd>
#include <stdint.h>

namespace Barry {

class Probe;

struct BXEXPORT ProbeResult
{
	friend class Probe;

	Usb::DeviceID* m_dev;
	unsigned char m_interface;
	Barry::Pin m_pin;
	Usb::EndpointPair m_ep;
	Usb::EndpointPair m_epModem;
	// Specifies if it's necessary to clear halt on the
	// endpoints before using them. On some devices such
	// as the 8830 it's essential to clear halt. On other
	// devices such as the Curve 8520 calling clear halt
	// can cause them to get into a state where they drop
	// packets.
	bool m_needClearHalt;
	// Specifies if it's necessary to call usb_set_altinterface()
	// before attempting to use the end points for this device.
	//
	// This can help to re-synchronize the state between the USB
	// host and the device. However it can also cause usb-storage
	// URBs to be lost on some device, so it's only used as a
	// last resort.
	bool m_needSetAltInterface;
	uint8_t m_zeroSocketSequence;
	std::string m_description;

	// data from a possible ConfigFile (filled in automatically by
	// the probe code if available)
	std::string m_cfgDeviceName;

private:
	// All ProbeResult objects should come from Probe, therefore
	// this constructor is private to force the issue.
	ProbeResult()
		: m_dev(0), m_interface(0), m_pin(0)
		, m_needClearHalt(false), m_needSetAltInterface(false)
		, m_zeroSocketSequence(0)
		{}

public:
	void DumpAll(std::ostream &os) const;
	bool HasIpModem() const { return m_epModem.IsComplete(); }

	bool operator==(const Barry::Pin &pin) const
	{
		return m_pin == pin;
	}
};

BXEXPORT std::ostream& operator<< (std::ostream &os, const ProbeResult &pr);


class BXEXPORT Probe
{
public:
	typedef std::vector<ProbeResult>		Results;

private:
	Usb::DeviceList m_devices;
	Results m_results;

	std::vector<std::string> m_fail_msgs;
	int m_fail_count;

	bool m_epp_override;
	Usb::EndpointPair m_epp;

	BXLOCAL bool CheckSize(const Data &data, unsigned int required);
	BXLOCAL bool ParsePIN(const Data &data, uint32_t &pin);
	BXLOCAL bool ParseDesc(const Data &data, std::string &desc);

protected:
	void ProbeMatching(int vendor, int product,
		const char *busname, const char *devname);
	void ProbeDevice(Usb::DeviceID* devid);
	void ProbeDeviceEndpoints(Usb::Device &dev, Usb::EndpointPairings &ed, ProbeResult &result);
	bool ProbePair(Usb::Device &dev, const Usb::EndpointPair &ep,
		uint32_t &pin, std::string &desc, uint8_t &zeroSocketSequence,
		bool &needClearHalt);
	bool ProbeModem(Usb::Device &dev, const Usb::EndpointPair &ep);

public:
	Probe(const char *busname = 0, const char *devname = 0,
		const Usb::EndpointPair *epp = 0);

	const Results& GetResults() const { return m_results; }

	int GetCount() const { return m_results.size(); }
	int GetFailCount() const { return m_fail_count; }

	const std::string& GetFailMsg(int index) const { return m_fail_msgs.at(index); }
	const ProbeResult& Get(int index) const { return m_results.at(index); }

	int FindActive(Barry::Pin pin = 0) const; // returns -1 if pin not found
						// or if no devices
	static int FindActive(const Results &results, Barry::Pin pin = 0);
	static int Find(const Results &results, Barry::Pin pin = 0);
};


} // namespace Barry

#endif

