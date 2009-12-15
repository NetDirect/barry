///
/// \file	deviceset.cc
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

#include "deviceset.h"
#include <string.h>
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////
// DeviceEntry class

DeviceEntry::DeviceEntry(const Barry::ProbeResult *result,
			group_ptr group,
			OpenSync::API *engine)
	: m_result(result)
	, m_group(group)
	, m_engine(engine)
{
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

		if( bp.GetPin().valid() ) {
			// double check for possible conflicting pin numbers
			if( pin.valid() ) {
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
	// load convenience values
	if( m_result )
		return m_result->m_cfgDeviceName;

	return std::string();
}

void DeviceEntry::SetConfigGroup(group_ptr group,
				OpenSync::API *engine)
{
	m_group = group;
	m_engine = engine;
}

//////////////////////////////////////////////////////////////////////////////
// DeviceSet class

/// Does a USB probe automatically
DeviceSet::DeviceSet(OpenSync::APISet &apiset)
	: m_apiset(apiset)
{
	Barry::Probe probe;
	m_results = probe.GetResults();
	LoadSet();
}

/// Skips the USB probe and uses the results set given
DeviceSet::DeviceSet(const Barry::Probe::Results &results,
				OpenSync::APISet &apiset)
	: m_apiset(apiset)
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
			if( result != m_results.end() ) {
				connected = &(*result);
			}

			// if no LoadError exceptions, add to configured list
			push_back( DeviceEntry(connected, g, &api) );

		}
		catch( Config::LoadError & ) {
			// if we catch LoadError, it just means that this
			// isn't a config that Barry Desktop can handle
			// so just skip it
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
			DeviceEntry item( &(*i), DeviceEntry::group_ptr(), 0 );
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
			return strcmp(a.GetPin().str().c_str(),
				b.GetPin().str().c_str()) < 0;
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

