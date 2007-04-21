//
// \file	environment.cc
//		Container / environment class for the sync module.
//

/*
    Copyright (C) 2006-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "environment.h"
#include "trace.h"
#include <glib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

//////////////////////////////////////////////////////////////////////////////
// static members

bool BarryEnvironment::LoadCache(const std::string &file, cache_type &cache)
{
	std::ifstream ifs(file.c_str());
	if( !ifs )
		return false;

	while( ifs ) {
		uint32_t recordId = 0;
		ifs >> recordId;
		if( recordId ) {
			cache[recordId] = false;
		}
	}
	return ifs.eof();
}

bool BarryEnvironment::SaveCache(const std::string &file, const cache_type &cache)
{
	std::ofstream ofs(file.c_str());
	if( !ofs )
		return false;

	cache_type::const_iterator i = cache.begin();
	for( ; i != cache.end(); ++i ) {
		ofs << i->first << std::endl;
	}
	return !ofs.bad() && !ofs.fail();
}


//////////////////////////////////////////////////////////////////////////////
// Public API

BarryEnvironment::BarryEnvironment(OSyncMember *pm)
	: member(pm),
	m_pin(0),
	m_SyncCalendar(false),
	m_SyncContacts(false),
	m_pCon(0)
{
	m_CalendarCacheFilename = m_ContactsCacheFilename =
		osync_member_get_configdir(pm);
	m_CalendarCacheFilename += "/barry_calendar_cache.txt";
	m_ContactsCacheFilename += "/barry_contacts_cache.txt";
}

BarryEnvironment::~BarryEnvironment()
{
	delete m_pCon;
}

void BarryEnvironment::Disconnect()
{
	delete m_pCon;
	m_pCon = 0;
}

bool BarryEnvironment::LoadCalendarCache()
{
	Trace trace("LoadCalendarCache");

	m_CalendarCache.clear();
	if( !LoadCache(m_CalendarCacheFilename, m_CalendarCache) ) {
		m_CalendarCache.clear();	// assume full sync
		trace.log("Load failed!");
		return false;
	}
	return true;
}

bool BarryEnvironment::LoadContactsCache()
{
	Trace trace("LoadContactsCache");

	m_ContactsCache.clear();
	if( !LoadCache(m_ContactsCacheFilename, m_ContactsCache) ) {
		m_ContactsCache.clear();
		trace.log("Load failed!");
		return false;
	}
	return true;
}

bool BarryEnvironment::SaveCalendarCache()
{
	Trace trace("SaveCalendarCache");
	return SaveCache(m_CalendarCacheFilename, m_CalendarCache);
}

bool BarryEnvironment::SaveContactsCache()
{
	Trace trace("SaveContactsCache");
	return SaveCache(m_ContactsCacheFilename, m_ContactsCache);
}

void BarryEnvironment::ClearDirtyFlags(Barry::RecordStateTable &table,
				const std::string &dbname)
{
	Trace trace("ClearDirtyFlags");

	unsigned int dbId = m_pCon->GetDBID(dbname);

	Barry::RecordStateTable::StateMapType::const_iterator i = table.StateMap.begin();
	for( ; i != table.StateMap.end(); ++i ) {
		if( i->second.Dirty ) {
			m_pCon->ClearDirty(dbId, i->first);
		}
	}
}

void BarryEnvironment::ClearCalendarDirtyFlags()
{
	Trace trace("ClearCalendarDirtyFlags");
	ClearDirtyFlags(m_CalendarTable, Barry::Calendar::GetDBName());
}

void BarryEnvironment::ClearContactsDirtyFlags()
{
	Trace trace("ClearContactsDirtyFlags");
	ClearDirtyFlags(m_ContactsTable, Barry::Contact::GetDBName());
}

void BarryEnvironment::ParseConfig(const char *data, int size)
{
	Trace trace("ParseConfig");

	m_ConfigData.assign(data, size);

	// The config data should contain:
	//    - PIN of device to sync with
	//    - or a flag that says "autoconfig with first device found"
	//      which will autodetect, and update the config
	//      automatically with the found PIN... all future syncs
	//      will then have a PIN
	//    - checkboxes for (both can be on):
	//         - sync calendar items
	//         - sync contacts

	std::istringstream iss(m_ConfigData);
	std::string line;
	while( std::getline(iss, line) ) {

		if( line[0] == '#' )
			continue;

		std::istringstream ils(line);
		int cal = 0, con = 0;
		ils >> std::hex >> m_pin >> cal >> con;

		std::ostringstream oss;
		oss << std::hex << m_pin;
		trace.log(oss.str().c_str());

		if( cal ) {
			m_SyncCalendar = true;
			trace.log("calendar syncing enabled");
		}

		if( con ) {
			m_SyncContacts = true;
			trace.log("contacts syncing enabled");
		}
	}
}

//void BarryEnvironment::BuildConfig()
//{
//	FIXME - build back into one long string
//}

