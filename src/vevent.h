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

#include "dll.h"
#include "vbase.h"
#include "vformat.h"
#include "r_calendar.h"
#include <stdint.h>
#include <string>

namespace Barry { namespace Sync {

//
// vCalendar
//
/// Class for converting between RFC 2445 iCalendar data format,
/// and the Barry::Calendar class.
///
class BXEXPORT vCalendar : public vBase
{
	// external reference
	vTimeConverter &m_vtc;

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
	explicit vCalendar(vTimeConverter &vtc);
	~vCalendar();

	const std::string&	ToVCal(const Barry::Calendar &cal);
	const Barry::Calendar&	ToBarry(const char *vcal, uint32_t RecordId);

	const std::string&	GetVCal() const { return m_vCalData; }
	const Barry::Calendar&	GetBarryCal() const { return m_BarryCal; }

	char* ExtractVCal();

	void Clear();

	// This is the v-name of the innermost BEGIN/END block
	static const char* GetVName() { return "VEVENT"; }
};

}} // namespace Barry::Sync

#endif

