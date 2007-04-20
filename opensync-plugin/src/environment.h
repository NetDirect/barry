//
// \file	environment.h
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

#ifndef __BARRY_SYNC_ENVIRONMENT_H__
#define __BARRY_SYNC_ENVIRONMENT_H__

#include <opensync/opensync.h>
#include <barry/barry.h>
#include <string>


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

	// device communication
	Barry::ProbeResult m_ProbeResult;
	Barry::Controller *m_pCon;

public:
	BarryEnvironment(OSyncMember *pm);
	~BarryEnvironment();

	void Disconnect();

	static bool LoadCache(const std::string &file, cache_type &cache);

	bool LoadCalendarCache();
	bool LoadContactsCache();
	void ParseConfig(const char *data, int size);
	void BuildConfig();
};

#endif

