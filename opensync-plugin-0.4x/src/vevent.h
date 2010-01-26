//
// \file	vevent.h
//		Conversion routines for vevents (VCALENDAR, etc)
//

/*
    Copyright (C) 2006-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_VEVENT_H__
#define __BARRY_SYNC_VEVENT_H__

#include <barry/barry.h>
#include <stdint.h>
#include <string>
#include "vbase.h"
#include "vformat.h"

// forward declarations
class BarryEnvironment;

//
// vCalendar
//
/// Class for converting between RFC 2445 iCalendar data format,
/// and the Barry::Calendar class.
///
class vCalendar : public vBase
{
	// data to pass to external requests
	char *m_gCalData;	// dynamic memory returned by vformat()... can
				// be used directly by the plugin, without
				// overmuch allocation and freeing (see Extract())
	std::string m_vCalData;	// copy of m_gCalData, for C++ use
	Barry::Calendar m_BarryCal;

	static const char *WeekDays[7];

	unsigned short GetMonthWeekNumFromBYDAY(const std::string& ByDay);
	unsigned short GetWeekDayIndexFromBYDAY(const std::string& ByDay);

protected:
	void RecurToVCal();
	void RecurToBarryCal(vAttr& rrule, time_t starttime);

	static unsigned short GetWeekDayIndex(const char *dayname);
	bool HasMultipleVEvents() const;

public:
	vCalendar();
	~vCalendar();

	const std::string&	ToVCal(const Barry::Calendar &cal);
	const Barry::Calendar&	ToBarry(const char *vcal, uint32_t RecordId);

	const std::string&	GetVCal() const { return m_vCalData; }
	const Barry::Calendar&	GetBarryCal() const { return m_BarryCal; }

	char* ExtractVCal();

	void Clear();
};


class VEventConverter
{
	char *m_Data;
	Barry::Calendar m_Cal;
	uint32_t m_RecordId;

public:
	VEventConverter();
	explicit VEventConverter(uint32_t newRecordId);
	~VEventConverter();

	// Transfers ownership of m_Data to the caller
	char* ExtractData();

	// Parses vevent data
	bool ParseData(const char *data);

	// Barry storage operator
	void operator()(const Barry::Calendar &rec);

	// Barry builder operator
	bool operator()(Barry::Calendar &rec, unsigned int dbId);

	// Handles calling of the Barry::Controller to fetch a specific
	// record, indicated by index (into the RecordStateTable).
	// Returns a g_malloc'd string of data containing the vevent20
	// data.  It is the responsibility of the caller to free it.
	// This is intended to be passed into the GetChanges() function.
	static char* GetRecordData(BarryEnvironment *env, unsigned int dbId,
		Barry::RecordStateTable::IndexType index);

	// Handles either adding or overwriting a calendar record,
	// given vevent20 data in data, and the proper environmebnt,
	// dbId, StateIndex.  Set add to true if adding.
	static bool CommitRecordData(BarryEnvironment *env, unsigned int dbId,
		Barry::RecordStateTable::IndexType StateIndex, uint32_t recordId,
		const char *data, bool add, std::string &errmsg);
};


#endif

