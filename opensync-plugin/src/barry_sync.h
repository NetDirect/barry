//
// \file	barry_sync.h
//		Opensync module for the USB Blackberry handheld
//

/*
    Copyright (C) 2006, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_H__
#define __BARRY_SYNC_H__

#include <opensync/opensync.h>
#include <barry/barry.h>
#include <string>
#include <fstream>

struct BarryEnvironment
{
public:
	typedef std::map<uint32_t, bool>			cache_type;

public:
	OSyncMember *member;

	// cache data
	std::string m_CalendarCacheFilename, m_ContactsCacheFilename;
	cache_type m_CalendarCache, m_ContactsCache;

	// device data
	Barry::RecordStateTable m_CalendarTable;
	Barry::RecordStateTable m_ContactsTable;

	// user config data
	std::string m_ConfigData;
	uint32_t m_pin;
	bool m_SyncCalendar;
	bool m_SyncContacts;

	//If you need a hashtable:
	OSyncHashTable *hashtable;

	// device communication
	Barry::ProbeResult m_ProbeResult;
	Barry::Controller *m_pCon;

public:
	BarryEnvironment(OSyncMember *pm)
		: member(pm),
		m_pin(0),
		m_SyncCalendar(false),
		m_SyncContacts(false),
		hashtable(0),
		m_pCon(0)
	{
		m_CalendarCacheFilename = m_ContactsCacheFilename =
			osync_member_get_configdir(pm);
		m_CalendarCacheFilename += "/barry_calendar_cache.txt";
		m_ContactsCacheFilename += "/barry_contacts_cache.txt";
	}

	~BarryEnvironment()
	{
		delete m_pCon;
	}

	void Disconnect()
	{
		delete m_pCon;
		m_pCon = 0;
	}

	static bool LoadCache(const std::string &file, cache_type &cache)
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
		return !ifs.bad() && !ifs.fail();
	}

	bool LoadCalendarCache()
	{
		m_CalendarCache.clear();
		if( !LoadCache(m_CalendarCacheFilename, m_CalendarCache) ) {
			m_CalendarCache.clear();	// assume full sync
			return false;
		}
		return true;
	}

	bool LoadContactsCache()
	{
		m_ContactsCache.clear();
		if( !LoadCache(m_ContactsCacheFilename, m_ContactsCache) ) {
			m_ContactsCache.clear();
			return false;
		}
		return true;
	}

	void ParseConfig(const char *data, int size)
	{
		m_ConfigData.assign(data, size);
//		FIXME - do some parsing...

		// The config data should contain:
		//    - PIN of device to sync with
		//    - or a flag that says "autoconfig with first device found"
		//      which will autodetect, and update the config
		//      automatically with the found PIN... all future syncs
		//      will then have a PIN
		//    - checkboxes for (both can be on):
		//         - sync calendar items
		//         - sync contacts
	}

//	void BuildConfig()
//	{
//		FIXME - build back into one long string
//	}
};



class Trace
{
	const char *text;
public:
	Trace(const char *t) : text(t)
	{
		osync_trace(TRACE_ENTRY, "barry_sync: %s", text);
	}

	~Trace()
	{
		osync_trace(TRACE_EXIT, "barry_sync: %s", text);
	}

	void log(const char *t)
	{
		osync_trace(TRACE_INTERNAL, "%s", t);
	}
};

#endif

