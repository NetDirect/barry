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
}

DatabaseSyncState::~DatabaseSyncState()
{
}

//
// Map2Uid
//
/// Converts record ID to string, since opensync 0.4x keeps the
/// same UID as we give it.
///
std::string DatabaseSyncState::Map2Uid(uint32_t recordId) const
{
	std::ostringstream oss;
	oss << std::dec << recordId;
	return oss.str();
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
	m_ContactsSync(info, "contacts"),
	m_JournalSync(info, "note")
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

	m_JournalSync.m_dbId = m_pDesktop->GetDBID(Barry::Memo::GetDBName());
	m_JournalSync.m_dbName = Barry::Memo::GetDBName();
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

bool BarryEnvironment::isConnected()
{
	if (m_pCon != 0)
		return true;

	return false;
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

void BarryEnvironment::ClearJournalDirtyFlags()
{
	Trace trace("ClearJournalDirtyFlags");
	ClearDirtyFlags(m_JournalSync.m_Table, Barry::Memo::GetDBName());
}

DatabaseSyncState* BarryEnvironment::GetSyncObject(OSyncChange *change)
{
	Trace trace("BarryEnvironment::GetSyncObject()");

	const char *name = osync_change_get_objtype(change);

	trace.logf("osync_change_get_objtype returns %s", name);

	if( strcmp(name, "event") == 0 ) {
		trace.log("return calendar object");

		return &m_CalendarSync;
	}
	else if( strcmp(name, "contact") == 0 ) {
		trace.log("return contact object");

		return &m_ContactsSync;
	}
	else if( strcmp(name, "note") == 0 ) {
		trace.log("return journal object");

		return &m_JournalSync;
	}

	trace.log("return none");

	return 0;
}

