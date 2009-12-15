///
/// \file	deviceset.h
///		Class which detects a set of available or known devices
///		in an opensync-able system.
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_DEVICE_SET_H__
#define __BARRY_DEVICE_SET_H__

#include "osconfig.h"
#include <iosfwd>

//
// DeviceEntry
//
/// Entry in the DeviceSet class.  This class must be STL container safe.
///
class DeviceEntry
{
public:
	typedef std::tr1::shared_ptr<OpenSync::Config::Group>	group_ptr;

private:
	// pointers to external data
	// pointers may be 0 if data is not available (such as when
	// a device is not connected, it would not have a ProbeResult)
	const Barry::ProbeResult *m_result;	// pointer to external data

	group_ptr m_group;			// may contain 0

	OpenSync::API *m_engine;		// may be 0

	std::string m_device_name;

protected:
	OpenSync::Config::Barry* FindBarry();	// returns pointer to the Barry
						// plugin object in m_group
						// or 0 if not available

public:
	DeviceEntry(const Barry::ProbeResult *result,
		group_ptr group,
		OpenSync::API *engine,
		const std::string &secondary_device_name = "");

	Barry::Pin GetPin() const;
	std::string GetDeviceName() const;
	bool IsConnected() const { return m_result; }
	bool IsConfigured() const { return m_group.get(); }

	const Barry::ProbeResult* GetProbeResult() { return m_result; }
	OpenSync::Config::Group* GetConfigGroup() { return m_group.get(); }
	OpenSync::API* GetEngine() { return m_engine; }
	const OpenSync::API* GetEngine() const { return m_engine; }

	void SetConfigGroup(group_ptr group, OpenSync::API *engine);
};

std::ostream& operator<< (std::ostream &os, const DeviceEntry &de);

//
// DeviceSet
//
/// This class detects known devices on an opensync-able system.
/// It will search for connected (USB) devices and devices that have been
/// configured in OpenSync (both 0.22 and 0.4x) but are not currently
/// connected.
///
/// For each device entry, it will know the following:
///	- pin
///	- device name (from barrybackup configs)
///	- whether connected or not
///	- whether configured for opensync or not... if so, it will also
///		keep track of:
///		- the app(s) it will sync with (in one sync group only)
///		- the version of the engine it is configured with
///
/// If a device is configured in both 0.4x and 0.22, then the first one
/// found is used.
///
/// Since this class needs to open and parse a lot of information during
/// construction anyway, it will store as much as possible, to allow for
/// editing in a GUI and saving it later.  The domain specific data
/// may be encapsulated in further classes.
///
class DeviceSet : public std::vector<DeviceEntry>
{
public:
	typedef std::vector<DeviceEntry>		base_type;
	typedef base_type::iterator			iterator;
	typedef base_type::const_iterator		const_iterator;

private:
	OpenSync::APISet &m_apiset;
	Barry::Probe::Results m_results;

protected:
	void LoadSet();
	void LoadConfigured(OpenSync::API &api);
	void LoadUnconfigured();
	void Sort();

public:
	/// Does a USB probe automatically
	explicit DeviceSet(OpenSync::APISet &apiset);

	/// Skips the USB probe and uses the results set given
	DeviceSet(const Barry::Probe::Results &results, OpenSync::APISet &apiset);

	iterator FindPin(const Barry::Pin &pin);
};

std::ostream& operator<< (std::ostream &os, const DeviceSet &ds);

#endif

