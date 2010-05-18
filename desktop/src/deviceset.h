///
/// \file	deviceset.h
///		Class which detects a set of available or known devices
///		in an opensync-able system.
///

/*
    Copyright (C) 2009-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

namespace Barry {
	class GlobalConfigFile;
}

//
// DeviceExtras
//
/// Config class to hold, load, and save device-related extras that are
/// not saved in either the Barry::ConfigFile or the OpenSync group
/// config.
///
/// These items are not stored in Barry::ConfigFile since they pertain
/// to a particular OpenSync sync group.  The user may have groups
/// configured outside of BarryDesktop.
class DeviceExtras
{
	Barry::Pin m_pin;

public:
	// config data... The Extras
	std::string m_favour_plugin_name;	// if empty, ask user
	time_t m_last_sync_time;

protected:
	std::string MakeBaseKey(const std::string &group_name);

public:
	explicit DeviceExtras(const Barry::Pin &pin);
	DeviceExtras(const Barry::Pin& pin,
		const Barry::GlobalConfigFile &config,
		const std::string &group_name);

	//
	// operations
	//
	void Load(const Barry::GlobalConfigFile &config,
		const std::string &group_name);
	void Save(Barry::GlobalConfigFile &config,
		const std::string &group_name);
};


//
// DeviceEntry
//
/// Entry in the DeviceSet class.  This class must be STL container safe.
///
class DeviceEntry
{
public:
	typedef std::tr1::shared_ptr<OpenSync::Config::Group>	group_ptr;
	typedef std::tr1::shared_ptr<DeviceExtras>		extras_ptr;

private:
	// pointers to external data
	// pointers may be 0 if data is not available (such as when
	// a device is not connected, it would not have a ProbeResult)
	const Barry::ProbeResult *m_result;	// pointer to external data

	group_ptr m_group;			// may contain 0
	OpenSync::API *m_engine;		// may be 0
	extras_ptr m_extras;			// may contain 0

	std::string m_device_name;

protected:
	OpenSync::Config::Barry* FindBarry();	// returns pointer to the Barry
						// plugin object in m_group
						// or 0 if not available

public:
	DeviceEntry(const Barry::GlobalConfigFile &config,
		const Barry::ProbeResult *result,
		group_ptr group,
		OpenSync::API *engine,
		const std::string &secondary_device_name = "");

	Barry::Pin GetPin() const;
	std::string GetDeviceName() const;
	bool IsConnected() const { return m_result; }
	bool IsConfigured() const { return m_group.get() && m_engine && m_group->AllConfigured(*m_engine); }
	std::string GetAppNames() const { return m_group.get() && m_engine ? m_group->GetAppNames() : ""; }

	/// Returns a string uniquely identifying this DeviceEntry
	std::string GetIdentifyingString() const;

	const Barry::ProbeResult* GetProbeResult() { return m_result; }
	OpenSync::Config::Group* GetConfigGroup() { return m_group.get(); }
	const OpenSync::Config::Group* GetConfigGroup() const { return m_group.get(); }
	OpenSync::API* GetEngine() { return m_engine; }
	const OpenSync::API* GetEngine() const { return m_engine; }

	DeviceExtras* GetExtras() { return m_extras.get(); }
	const DeviceExtras* GetExtras() const { return m_extras.get(); }

	void SetConfigGroup(group_ptr group, OpenSync::API *engine,
		extras_ptr extras);
	void SetDeviceName(const std::string &name) { m_device_name = name; }
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
/// If a device is configured in both 0.4x and 0.22, or even in two
/// groups in one engine, then all are loaded.  Use FindDuplicates()
/// and KillDuplicates() to sort that out with the user's help.
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
	typedef std::vector<iterator>			subset_type;

private:
	const Barry::GlobalConfigFile &m_config;
	OpenSync::APISet &m_apiset;
	Barry::Probe::Results m_results;

protected:
	void LoadSet();
	void LoadConfigured(OpenSync::API &api);
	void LoadUnconfigured();
	void Sort();

public:
	/// Does a USB probe automatically
	DeviceSet(const Barry::GlobalConfigFile &config,
		OpenSync::APISet &apiset);

	/// Skips the USB probe and uses the results set given
	DeviceSet(const Barry::GlobalConfigFile &config,
		OpenSync::APISet &apiset,
		const Barry::Probe::Results &results);

	iterator FindPin(const Barry::Pin &pin);
	const_iterator FindPin(const Barry::Pin &pin) const;
	static subset_type::const_iterator FindPin(const subset_type &subset, const Barry::Pin &pin);
	static std::string Subset2String(const subset_type &set);
	subset_type String2Subset(const std::string &list);

	/// Searches for DeviceEntry's in the set that have the same
	/// pin number.  This is most likely due to OpenSync having
	/// multiple configurations with the same device.  This
	/// function returns a vector of iterators into the DeviceSet,
	/// or an empty vector if no duplicates are found.
	///
	/// You can solve the duplicate by erase()ing the chosen
	/// iterator.  Call this function multiple times to find
	/// all the duplicates.
	subset_type FindDuplicates();

	/// Safely removes all entries that are referenced by
	/// the iterators in dups.  Note that a call to erase()
	/// invalidates the other iterators, so this function
	/// does it safely.
	void KillDuplicates(const subset_type &dups);
};

std::ostream& operator<< (std::ostream &os, const DeviceSet &ds);

#endif

