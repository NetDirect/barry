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
#include <opensync/data/opensync_change.h>


//////////////////////////////////////////////////////////////////////////////
// DatabaseSyncState

DatabaseSyncState::DatabaseSyncState(OSyncPluginInfo *pi, const char *description)
	: m_Sync(false),
	m_Desc(description)
{
	m_CacheFilename = m_MapFilename =
		osync_plugin_info_get_configdir(pi);
	m_CacheFilename += "/barry_" + m_Desc + "_cache.txt";
	m_MapFilename += "/barry_" + m_Desc + "_idmap.txt";
}

DatabaseSyncState::~DatabaseSyncState()
{
}

bool DatabaseSyncState::LoadCache()
{
	Trace trace("LoadCache", m_Desc.c_str());

	m_Cache.clear();
	std::ifstream ifs(m_CacheFilename.c_str());
	if( !ifs )
		return false;

	while( ifs ) {
		uint32_t recordId = 0;
		ifs >> recordId;
		if( recordId ) {
			m_Cache[recordId] = false;
		}
	}

	if( !ifs.eof() ) {
		m_Cache.clear();	// assume full sync
		trace.log("Load failed!");
		return false;
	}
	return true;
}

bool DatabaseSyncState::SaveCache()
{
	Trace trace("SaveCache", m_Desc.c_str());

	std::ofstream ofs(m_CacheFilename.c_str());
	if( !ofs )
		return false;

	cache_type::const_iterator i = m_Cache.begin();
	for( ; i != m_Cache.end(); ++i ) {
		ofs << i->first << std::endl;
	}
	return !ofs.bad() && !ofs.fail();
}

bool DatabaseSyncState::LoadMap()
{
	return m_IdMap.Load(m_MapFilename.c_str());
}

bool DatabaseSyncState::SaveMap()
{
	return m_IdMap.Save(m_MapFilename.c_str());
}

// cycle through the map and search the state table for each rid,
// and remove any that do not exist
void DatabaseSyncState::CleanupMap()
{
	idmap::iterator i = m_IdMap.begin();
	for( ; i != m_IdMap.end(); ++i ) {
		if( !m_Table.GetIndex(i->second) ) {
			// Record Id does not exist in state table, so it is
			// not needed anymore in the ID map
			m_IdMap.Unmap(i);
		}
	}
}

unsigned long DatabaseSyncState::GetMappedRecordId(const std::string &uid)
{
	Trace trace("DatabaseSyncState::GetMappedRecordId()", m_Desc.c_str());

	// if already in map, use the matching rid
	idmap::const_iterator it;
	if( m_IdMap.UidExists(uid, &it) ) {
		trace.logf("found existing uid in map: %lu", it->second);
		return it->second;
	}

	// nothing in the map, so try to convert the string to a number
	unsigned long RecordId;
	if( sscanf(uid.c_str(), "%lu", &RecordId) != 0 ) {
		trace.logf("parsed uid as: %lu", RecordId);
		if( m_IdMap.Map(uid, RecordId) != m_IdMap.end() )
			return RecordId;

		trace.logf("parsed uid already exists in map, skipping");
	}

	// create one of our own, if we get here...
	// do this in a loop to keep going until we find an ID that's unique
	do {
		RecordId = m_Table.MakeNewRecordId();
	} while( m_IdMap.Map(uid, RecordId) == m_IdMap.end() );

	trace.logf("made new record id: %lu", RecordId);
	return RecordId;
}



//////////////////////////////////////////////////////////////////////////////
// BarryEnvironment Public API

BarryEnvironment::BarryEnvironment(OSyncPluginInfo *pi)
	: m_pin(0),
	m_pCon(0),
	m_CalendarSync(pi, "calendar"),
	m_ContactsSync(pi, "contacts")
{
}

BarryEnvironment::~BarryEnvironment()
{
	delete m_pCon;
}

void BarryEnvironment::OpenDesktop(Barry::ProbeResult &result)
{
	if( m_pCon )
		throw std::logic_error("Desktop already opened");

	m_pCon = new Barry::Controller(result);
	m_pCon->OpenMode(Barry::Controller::Desktop);
}

void BarryEnvironment::Disconnect()
{
	delete m_pCon;
	m_pCon = 0;
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
	ClearDirtyFlags(m_CalendarSync.m_Table, Barry::Calendar::GetDBName());
}

void BarryEnvironment::ClearContactsDirtyFlags()
{
	Trace trace("ClearContactsDirtyFlags");
	ClearDirtyFlags(m_ContactsSync.m_Table, Barry::Contact::GetDBName());
}

DatabaseSyncState* BarryEnvironment::GetSyncObject(OSyncChange *change)
{
	Trace trace("BarryEnvironment::GetSyncObject()");

	const char *name = osync_change_get_objtype(change);
	if( strcmp(name, "event") == 0 ) {
		return &m_CalendarSync;
	}
	else if( strcmp(name, "contact") == 0 ) {
		return &m_ContactsSync;
	}
	else {
		return 0;
	}
}

void BarryEnvironment::ParseConfig(const char *data)
{
	Trace trace("ParseConfig");

	m_ConfigData = data;

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
			m_CalendarSync.m_Sync = true;
			trace.log("calendar syncing enabled");
		}

		if( con ) {
			m_ContactsSync.m_Sync = true;
			trace.log("contacts syncing enabled");
		}
	}
}

//void BarryEnvironment::BuildConfig()
//{
//	FIXME - build back into one long string
//}

