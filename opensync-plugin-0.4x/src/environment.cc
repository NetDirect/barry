//
// \file	environment.cc
//		Container / environment class for the sync module.
//

/*
    Copyright (C) 2006-2013, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <unistd.h>
#include "i18n.h"

using namespace Barry;

//////////////////////////////////////////////////////////////////////////////
// DatabaseSyncState

DatabaseSyncState::DatabaseSyncState(OSyncPluginInfo *info, const char *description)
	: sink(0),
	m_dbId(0),
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
	member(0),
	info(info),
	m_pin(-1),
	m_DebugMode(false),
	m_CalendarSync(info, "calendar"),
	m_ContactsSync(info, "contacts"),
	m_JournalSync(info, "note"),
	m_TodoSync(info, "todo")
{
}

BarryEnvironment::~BarryEnvironment()
{
}

void BarryEnvironment::DoConnect()
{
	if( !m_con.get() )
		throw std::logic_error(_("Tried to use empty Connector"));

	m_con->Connect();

	// Save the DBIDs and DBNames of the databases we will work with
	if( m_CalendarSync.m_Sync ) {
		m_CalendarSync.m_dbName = Barry::Calendar::GetDBName();
		m_CalendarSync.m_dbId = m_con->GetDesktop().GetDBID(Barry::Calendar::GetDBName());
	}

	if( m_ContactsSync.m_Sync ) {
		m_ContactsSync.m_dbName = Barry::Contact::GetDBName();
		m_ContactsSync.m_dbId = m_con->GetDesktop().GetDBID(Barry::Contact::GetDBName());
	}

	if( m_JournalSync.m_Sync ) {
		m_JournalSync.m_dbName = Barry::Memo::GetDBName();
		m_JournalSync.m_dbId = m_con->GetDesktop().GetDBID(Barry::Memo::GetDBName());
	}

	if( m_TodoSync.m_Sync ) {
		m_TodoSync.m_dbName = Barry::Task::GetDBName();
		m_TodoSync.m_dbId = m_con->GetDesktop().GetDBID(Barry::Task::GetDBName());
	}
}

void BarryEnvironment::SetPassword(const std::string &password)
{
	m_password = password;
	if( m_con.get() )
		m_con->SetPassword(password.c_str());
}

void BarryEnvironment::Connect(const Barry::ProbeResult &result)
{
	m_con.reset(new DesktopConnector(m_password.c_str(), "UTF-8", result));
	DoConnect();
}

void BarryEnvironment::Reconnect()
{
	m_con->Reconnect(2);
}

void BarryEnvironment::Disconnect()
{
	m_con->Disconnect();
}

bool BarryEnvironment::isConnected()
{
	return m_con.get() && m_con->IsConnected();
}

void BarryEnvironment::ReconnectForDirtyFlags()
{
	m_con->ReconnectForDirtyFlags();
}

void BarryEnvironment::RequireDirtyReconnect()
{
	m_con->RequireDirtyReconnect();
}

void BarryEnvironment::ClearDirtyFlags(Barry::RecordStateTable &table,
				const std::string &dbname)
{
	Trace trace("ClearDirtyFlags");

	unsigned int dbId = m_con->GetDesktop().GetDBID(dbname);

	Barry::RecordStateTable::StateMapType::const_iterator i = table.StateMap.begin();
	for( ; i != table.StateMap.end(); ++i ) {
		if( i->second.Dirty ) {
			trace.logf(_("Clearing dirty flag for db %u, index %u"),
				dbId, i->first);
			m_con->GetDesktop().ClearDirty(dbId, i->first);
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

void BarryEnvironment::ClearTodoDirtyFlags()
{
	Trace trace("ClearTodoDirtyFlags");
	ClearDirtyFlags(m_TodoSync.m_Table, Barry::Task::GetDBName());
}

DatabaseSyncState* BarryEnvironment::GetSyncObject(OSyncChange *change)
{
	Trace trace("BarryEnvironment::GetSyncObject()");

	const char *name = osync_change_get_objtype(change);

	trace.logf(_("osync_change_get_objtype returns %s"), name);

	if( strcmp(name, "event") == 0 ) {
		trace.log(_("return calendar object"));

		return &m_CalendarSync;
	}
	else if( strcmp(name, "contact") == 0 ) {
		trace.log(_("return contact object"));

		return &m_ContactsSync;
	}
	else if( strcmp(name, "note") == 0 ) {
		trace.log(_("return journal object"));

		return &m_JournalSync;
	}
	else if( strcmp(name, "todo") == 0 ) {
		trace.log(_("return todo object"));

		return &m_TodoSync;
	}

	trace.log(_("return none"));

	return 0;
}

