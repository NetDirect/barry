///
/// \file	deviceset.cc
///		Class which detects a set of available or known devices
///		in an opensync-able system.
///

/*
    Copyright (C) 2009-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "deviceset.h"
#include <barry/barry.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <iomanip>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// DeviceExtras class

DeviceExtras::DeviceExtras(const Barry::Pin &pin)
	: m_pin(pin)
	, m_last_sync_time(0)
	, m_sync_types(PST_NONE)
{
}

DeviceExtras::DeviceExtras(const Barry::Pin & pin,
				const Barry::GlobalConfigFile &config,
				const std::string &group_name)
	: m_pin(pin)
	, m_last_sync_time(0)
{
	Load(config, group_name);
}

std::string DeviceExtras::MakeBaseKey(const std::string &group_name)
{
	ostringstream oss;
	oss << m_pin.Str() << "-" << group_name << "-";
	return oss.str();
}

void DeviceExtras::Load(const Barry::GlobalConfigFile &config,
			const std::string &group_name)
{
	string key = MakeBaseKey(group_name);
	m_favour_plugin_name = config.GetKey(key + "FavourPlugin");

	string num = config.GetKey(key + "LastSyncTime");
	m_last_sync_time = atol(num.c_str());

	using OpenSync::Config::PSTString2Type;
	using OpenSync::Config::PSTType2String;
	string stypes = config.GetKey(key + "SyncTypes",
		PSTType2String(PST_ALL));
	m_sync_types = PSTString2Type(stypes);
}

void DeviceExtras::Save(Barry::GlobalConfigFile &config,
			const std::string &group_name)
{
	string key = MakeBaseKey(group_name);
	config.SetKey(key + "FavourPlugin", m_favour_plugin_name);

	ostringstream oss;
	oss << m_last_sync_time;
	config.SetKey(key + "LastSyncTime", oss.str());

	config.SetKey(key + "SyncTypes",
		OpenSync::Config::PSTType2String(m_sync_types));
}


//////////////////////////////////////////////////////////////////////////////
// DeviceEntry class

DeviceEntry::DeviceEntry(const Barry::GlobalConfigFile &config,
			const Barry::ProbeResult *result,
			group_ptr group,
			OpenSync::API *engine,
			const std::string &secondary_device_name)
	: m_result(result)
	, m_group(group)
	, m_engine(engine)
	, m_device_name(secondary_device_name)
{
	// just make sure that our device name has something in it
	if( !m_device_name.size() && m_result )
		m_device_name = m_result->m_cfgDeviceName;

	// load the extras if available
	Barry::Pin pin = GetPin();
	if( pin.Valid() && IsConfigured() ) {
		m_extras.reset( new DeviceExtras(pin, config,
					m_group->GetGroupName()) );
	}
}

// returns pointer to the Barry plugin object in m_group
// or 0 if not available
OpenSync::Config::Barry* DeviceEntry::FindBarry()
{
	if( m_group.get() && m_group->HasBarryPlugins() )
		return &m_group->GetBarryPlugin();
	return 0;
}

Barry::Pin DeviceEntry::GetPin() const
{
	Barry::Pin pin;

	// load convenience values
	if( m_result ) {
		pin = m_result->m_pin;
	}

	if( m_group.get() && m_group->HasBarryPlugins() ) {
		const OpenSync::Config::Barry &bp = m_group->GetBarryPlugin();

		if( bp.GetPin().Valid() ) {
			// double check for possible conflicting pin numbers
			if( pin.Valid() ) {
				if( pin != bp.GetPin() ) {
					throw std::logic_error("Probe pin != group pin in DeviceEntry");
				}
			}

			// got a valid pin, save it
			pin = bp.GetPin();
		}
	}

	return pin;
}

std::string DeviceEntry::GetDeviceName() const
{
	if( m_device_name.size() )
		return m_device_name;
	else if( m_result )
		return m_result->m_cfgDeviceName;
	else
		return std::string();
}

std::string DeviceEntry::GetIdentifyingString() const
{
	ostringstream oss;

	oss << GetPin().Str();
	string name = GetDeviceName();
	if( name.size() )
		oss << " (" << name << ")";

	if( IsConfigured() )
		oss << ", Group: " << GetConfigGroup()->GetGroupName();
	else
		oss << ", Not configured";

	if( GetEngine() )
		oss << ", Engine: " << GetEngine()->GetVersion();

	return oss.str();
}

void DeviceEntry::SetConfigGroup(group_ptr group,
				OpenSync::API *engine,
				extras_ptr extras)
{
	m_group = group;
	m_engine = engine;
	m_extras = extras;
}

std::ostream& operator<< (std::ostream &os, const DeviceEntry &de)
{
	os << setfill(' ') << setw(8) << de.GetPin().Str();
	os << "|" << setfill(' ') << setw(35) << de.GetDeviceName();
	os << "|" << setfill(' ') << setw(4) << (de.IsConnected() ? "yes" : "no");
	os << "|" << setfill(' ') << setw(4) << (de.IsConfigured() ? "yes" : "no");
	os << "|" << setfill(' ') << setw(7)
		<< (de.GetEngine() ? de.GetEngine()->GetVersion() : "");
	return os;
}

//////////////////////////////////////////////////////////////////////////////
// DeviceSet class

/// Does a USB probe automatically
DeviceSet::DeviceSet(const Barry::GlobalConfigFile &config,
			OpenSync::APISet &apiset)
	: m_config(config)
	, m_apiset(apiset)
{
	Barry::Probe probe;
	m_results = probe.GetResults();
	LoadSet();
}

/// Skips the USB probe and uses the results set given
DeviceSet::DeviceSet(const Barry::GlobalConfigFile &config,
			OpenSync::APISet &apiset,
			const Barry::Probe::Results &results)
	: m_config(config)
	, m_apiset(apiset)
	, m_results(results)
{
	LoadSet();
}

void DeviceSet::LoadSet()
{
	if( m_apiset.os40() )
		LoadConfigured(*m_apiset.os40());
	if( m_apiset.os22() )
		LoadConfigured(*m_apiset.os22());
	LoadUnconfigured();
	Sort();
}

/// Constructor helper function.  Adds configured DeviceEntry's to the set.
/// Does no sorting.
void DeviceSet::LoadConfigured(OpenSync::API &api)
{
	using namespace OpenSync;

	//
	// we already have the connected devices in m_results, so
	// load every Barry-related group that exists in the given API
	//

	// get group list
	string_list_type groups;
	api.GetGroupNames(groups);

	// for each group
	for( string_list_type::iterator b = groups.begin(); b != groups.end(); ++b ) {
		try {
			// load the group via Config::Group
			DeviceEntry::group_ptr g( new Config::Group(*b, api,
				OSCG_THROW_ON_UNSUPPORTED |
				OSCG_THROW_ON_NO_BARRY |
				OSCG_THROW_ON_MULTIPLE_BARRIES) );

			// now that we have a config group, check to see
			// if the pin in the group's Barry plugin is
			// available in the probe results... if so, it is
			// also connected
			OpenSync::Config::Barry &plugin = g->GetBarryPlugin();
			Barry::Probe::Results::iterator result =
				std::find(m_results.begin(), m_results.end(),
					plugin.GetPin());
			const Barry::ProbeResult *connected = 0;
			string dev_name;
			if( result != m_results.end() ) {
				connected = &(*result);
				dev_name = connected->m_cfgDeviceName;
			}
			else {
				// the device is not connected, so do a
				// special load of a possible device config
				// to load the device name
				Barry::ConfigFile cf(plugin.GetPin());
				dev_name = cf.GetDeviceName();
			}

			// if no LoadError exceptions, add to configured list
			push_back( DeviceEntry(m_config, connected, g, &api,
						dev_name) );

		}
		catch( Config::LoadError &le ) {
			// if we catch LoadError, it just means that this
			// isn't a config that Barry Desktop can handle
			// so just log and skip it
			barryverbose("DeviceSet::LoadConfigured: " << le.what());
		}
	}
}

void DeviceSet::LoadUnconfigured()
{
	// cycle through the probe results, and add any devices for
	// which their pins do not yet exist in the list
	for( Barry::Probe::Results::const_iterator i = m_results.begin();
		i != m_results.end();
		++i )
	{
		iterator p = FindPin(i->m_pin);
		if( p == end() ) {
			// this pin isn't in the list yet, so add it
			// as an unconfigured item

			// create the DeviceEntry with a null group_ptr
			DeviceEntry item( m_config, &(*i),
					DeviceEntry::group_ptr(), 0 );
			push_back( item );
		}
	}
}

/// Constructor helper function.  Loads the device list.  Sort by
/// pin number, but in the following groups:
///
///	- configured first (both connected and unconnected)
///	- unconfigured but connected second
///
/// This should preserve the sort order across multiple loads, to keep
/// a relatively consistent listing for the user.
///
namespace {
	bool DeviceEntryCompare(const DeviceEntry &a, const DeviceEntry &b)
	{
		if( a.IsConfigured() == b.IsConfigured() )
			return strcmp(a.GetPin().Str().c_str(),
				b.GetPin().Str().c_str()) < 0;
		else
			return a.IsConfigured();
	}
}
void DeviceSet::Sort()
{
	std::sort(begin(), end(), &DeviceEntryCompare);
}

DeviceSet::iterator DeviceSet::FindPin(const Barry::Pin &pin)
{
	for( iterator i = begin(); i != end(); ++i ) {
		if( i->GetPin() == pin )
			return i;
	}
	return end();
}

DeviceSet::const_iterator DeviceSet::FindPin(const Barry::Pin &pin) const
{
	for( const_iterator i = begin(); i != end(); ++i ) {
		if( i->GetPin() == pin )
			return i;
	}
	return end();
}

DeviceSet::subset_type::const_iterator DeviceSet::FindPin(
	const DeviceSet::subset_type &subset, const Barry::Pin &pin)
{
	for( subset_type::const_iterator i = subset.begin();
		i != subset.end();
		++i )
	{
		if( (*i)->GetPin() == pin )
			return i;
	}
	return subset.end();
}

std::string DeviceSet::Subset2String(const DeviceSet::subset_type &set)
{
	std::string list;

	subset_type::const_iterator i = set.begin();
	for( ; i != set.end(); ++i ) {
		if( list.size() )
			list += " ";
		list += (*i)->GetPin().Str();
	}
	return list;
}

DeviceSet::subset_type DeviceSet::String2Subset(const std::string &list)
{
	subset_type set;

	istringstream iss(list);
	Barry::Pin pin;

	while( iss >> pin ) {
		if( !pin.Valid() )
			continue;

		// search for pin in device set
		iterator i = FindPin(pin);
		if( i != end() )
			set.push_back(i);
	}
	return set;
}

DeviceSet::subset_type DeviceSet::FindDuplicates()
{
	subset_type dups;

	for( iterator i = begin(); i != end(); ++i ) {

		// start with this PIN
		dups.push_back(i);

		// search for a duplicate, and add all dups found
		for( iterator j = begin(); j != end(); ++j ) {
			// skip ourselves
			if( j == i )
				continue;

			if( i->GetPin() == j->GetPin() ) {
				// found a duplicate
				dups.push_back(j);
			}
		}

		// if we have multiple iterators in dups, we're done
		if( dups.size() > 1 )
			return dups;

		// else, start over
		dups.clear();
	}

	return dups;
}

void DeviceSet::KillDuplicates(const subset_type &dups)
{
	// anything to do?
	if( dups.size() == 0 )
		return;	// nope

	// only one?
	if( dups.size() == 1 ) {
		erase(dups[0]);
		return;
	}

	// ok, we have multiple dups to erase, so we need to make
	// a copy of ourselves and skip all matching iterators,
	// then copy the result back to this
	base_type copy;
	for( iterator i = begin(); i != end(); ++i ) {
		if( find(dups.begin(), dups.end(), i) == dups.end() ) {
			// not in the dups list, copy it
			copy.push_back(*i);
		}
	}

	// copy back
	clear();
	base_type::operator=(copy);
}

std::ostream& operator<< (std::ostream &os, const DeviceSet &ds)
{
	os << "  PIN   |       Device Name                 |Con |Cfg |Engine\n";
	os << "--------+-----------------------------------+----+----+-------\n";

	for( DeviceSet::const_iterator i = ds.begin(); i != ds.end(); ++i )
		os << *i << "\n" << i->GetIdentifyingString() << endl;
	return os;
}

