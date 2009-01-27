//
// \file	environment.cc
//		Container / environment class for the sync module.
//

/*
    Copyright (C) 2006-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <opensync/opensync.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-version.h>

#include "environment.h"
#include "trace.h"
#include <glib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string.h>


//////////////////////////////////////////////////////////////////////////////
// DatabaseSyncState

DatabaseSyncState::DatabaseSyncState(OSyncPluginInfo *info, const char *description)
	: m_dbId(0),
	m_Sync(false),
	m_Desc(description)
{
	m_CacheFilename = m_MapFilename = osync_plugin_info_get_configdir(info);
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

//
// Map2Uid
//
/// Searches for the given record ID, and returns the mapped UID.  If not
/// found, it creates a new UID and returns it without mapping it.
///
std::string DatabaseSyncState::Map2Uid(uint32_t recordId) const
{
	// search the idmap for the UID
	std::string uid;
	idmap::const_iterator mapped_id;
	if( m_IdMap.RidExists(recordId, &mapped_id) ) {
		uid = mapped_id->first;
	}
	else {
		// not mapped, map it ourselves
		char *puid = g_strdup_printf("%s-%u", m_Desc.c_str(), recordId);
		uid = puid;
		g_free(puid);
	}
	return uid;
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

BarryEnvironment::BarryEnvironment(OSyncPluginInfo *info):
	info(info),
	m_pin(-1),
	m_DebugMode(false),
	m_password(""),
	m_IConverter("UTF-8"),
	m_pCon(0),
	m_pDesktop(0),
	m_CalendarSync(info, "calendar"),
	m_ContactsSync(info, "contacts")
{
}

BarryEnvironment::~BarryEnvironment()
{
	delete m_pDesktop;
	delete m_pCon;
}

void BarryEnvironment::DoConnect()
{
	// Create controller
	m_pCon = new Barry::Controller(m_ProbeResult);
	m_pDesktop = new Barry::Mode::Desktop(*m_pCon, m_IConverter);
	m_pDesktop->Open(m_password.c_str());

	// Save the DBIDs and DBNames of the databases we will work with
	m_CalendarSync.m_dbName = Barry::Calendar::GetDBName();
	m_CalendarSync.m_dbId = m_pDesktop->GetDBID(Barry::Calendar::GetDBName());

	m_ContactsSync.m_dbId = m_pDesktop->GetDBID(Barry::Contact::GetDBName());
	m_ContactsSync.m_dbName = Barry::Contact::GetDBName();
}

void BarryEnvironment::Connect(const Barry::ProbeResult &result)
{
	Disconnect();

	// save result in case we need to reconnect later
	m_ProbeResult = result;

	DoConnect();
}

void BarryEnvironment::Reconnect()
{
	Disconnect();

	// FIXME - temporary fix for odd reconnect message... without this
	// probe, the reconnect will often fail on newer Blackberries
	// due to an unexpected close socket message.  It is unclear
	// if this is really a message from the device, but until then,
	// we add this probe.
	{
		Barry::Probe probe;
		int i = probe.FindActive(m_ProbeResult.m_pin);
		if( i != -1 )
			m_ProbeResult = probe.Get(i);
	}

	DoConnect();
}

void BarryEnvironment::Disconnect()
{
	delete m_pDesktop;
	m_pDesktop = 0;

	delete m_pCon;
	m_pCon = 0;
}

void BarryEnvironment::ClearDirtyFlags(Barry::RecordStateTable &table,
				const std::string &dbname)
{
	Trace trace("ClearDirtyFlags");

	unsigned int dbId = m_pDesktop->GetDBID(dbname);

	Barry::RecordStateTable::StateMapType::const_iterator i = table.StateMap.begin();
	for( ; i != table.StateMap.end(); ++i ) {
		if( i->second.Dirty ) {
			trace.logf("Clearing dirty flag for db %u, index %u",
				dbId, i->first);
			m_pDesktop->ClearDirty(dbId, i->first);
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


//void BarryEnvironment::BuildConfig()
//{
//	FIXME - build back into one long string
//}

