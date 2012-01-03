///
/// \file	r_recur_base.cc
///		Base class for recurring calendar event data.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "r_recur_base.h"
#include "protostructs.h"
#include "error.h"
#include "endian.h"
#include "time.h"
#include "ios_state.h"
#include <string.h>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;


#define FIELDCODE_RECURRENCE_DATA	0x0c

namespace Barry {


///////////////////////////////////////////////////////////////////////////////
// RecurBase class, static members

unsigned char RecurBase::WeekDayProto2Rec(uint8_t raw_field)
{
	// Note: this simple copy is only possible since
	// the CAL_WD_* constants are the same as CRDF_WD_* constants.
	// If this ever changes, this code will need to change.
	return raw_field;
}

uint8_t RecurBase::WeekDayRec2Proto(unsigned char weekdays)
{
	// Note: this simple copy is only possible since
	// the CAL_WD_* constants are the same as CRDF_WD_* constants.
	// If this ever changes, this code will need to change.
	return weekdays;
}


///////////////////////////////////////////////////////////////////////////////
// RecurBase class

RecurBase::RecurBase()
{
	Clear();
}

RecurBase::~RecurBase()
{
}

bool RecurBase::ParseField(uint8_t type,
			   const unsigned char *data,
			   size_t size,
			   const IConverter *ic)
{
	// handle special cases
	switch( type )
	{
	case FIELDCODE_RECURRENCE_DATA:
		if( size >= CALENDAR_RECURRENCE_DATA_FIELD_SIZE ) {
			// good data
			ParseRecurrenceData(data);
		}
		else {
			// not enough data!
			throw Error("RecurBase::ParseField: not enough data in recurrence data field");
		}
		return true;
	}

	// unknown field
	return false;
}

// this function assumes the size has already been checked
void RecurBase::ParseRecurrenceData(const void *data)
{
	const CalendarRecurrenceDataField *rec =
		(const CalendarRecurrenceDataField*) data;

	Interval = btohs(rec->interval);
	if( Interval < 1 )
		Interval = 1;	// must always be >= 1

	if( rec->endTime == 0xffffffff ) {
		Perpetual = true;
	}
	else {
		RecurringEndTime = min2time(rec->endTime);
		Perpetual = false;
	}

	switch( rec->type )
	{
	case CRDF_TYPE_DAY:
		RecurringType = Day;
		// no extra data
		break;

	case CRDF_TYPE_MONTH_BY_DATE:
		RecurringType = MonthByDate;
		DayOfMonth = rec->u.month_by_date.monthDay;
		break;

	case CRDF_TYPE_MONTH_BY_DAY:
		RecurringType = MonthByDay;
		DayOfWeek = rec->u.month_by_day.weekDay;
		WeekOfMonth = rec->u.month_by_day.week;
		break;

	case CRDF_TYPE_YEAR_BY_DATE:
		RecurringType = YearByDate;
		DayOfMonth = rec->u.year_by_date.monthDay;
		MonthOfYear = rec->u.year_by_date.month;
		break;

	case CRDF_TYPE_YEAR_BY_DAY:
		RecurringType = YearByDay;
		DayOfWeek = rec->u.year_by_day.weekDay;
		WeekOfMonth = rec->u.year_by_day.week;
		MonthOfYear = rec->u.year_by_day.month;
		break;

	case CRDF_TYPE_WEEK:
		RecurringType = Week;
		WeekDays = WeekDayProto2Rec(rec->u.week.days);
		break;

	default:
		eout("Unknown recurrence data type: 0x"
			<< setbase(16) << (unsigned int) rec->type);
		throw Error("Unknown recurrence data type");
	}

	Recurring = true;
}

// this function assumes there is CALENDAR_RECURRENCE_DATA_FIELD_SIZE bytes
// available in data
void RecurBase::BuildRecurrenceData(time_t StartTime, void *data) const
{
	if( !Recurring )
		throw Error("RecurBase::BuildRecurrenceData: Attempting to build recurrence data on non-recurring record.");

	CalendarRecurrenceDataField *rec = (CalendarRecurrenceDataField*) data;

	// set all to zero
	memset(data, 0, CALENDAR_RECURRENCE_DATA_FIELD_SIZE);

	rec->interval = htobs(Interval);
	rec->startTime = time2min(StartTime);
	if( Perpetual )
		rec->endTime = 0xffffffff;
	else
		rec->endTime = time2min(RecurringEndTime);

	switch( RecurringType )
	{
	case Day:
		rec->type = CRDF_TYPE_DAY;
		// no extra data
		break;

	case MonthByDate:
		rec->type = CRDF_TYPE_MONTH_BY_DATE;
		rec->u.month_by_date.monthDay = DayOfMonth;
		break;

	case MonthByDay:
		rec->type = CRDF_TYPE_MONTH_BY_DAY;
		rec->u.month_by_day.weekDay = DayOfWeek;
		rec->u.month_by_day.week = WeekOfMonth;
		break;

	case YearByDate:
		rec->type = CRDF_TYPE_YEAR_BY_DATE;
		rec->u.year_by_date.monthDay = DayOfMonth;
		rec->u.year_by_date.month = MonthOfYear;
		break;

	case YearByDay:
		rec->type = CRDF_TYPE_YEAR_BY_DAY;
		rec->u.year_by_day.weekDay = DayOfWeek;
		rec->u.year_by_day.week = WeekOfMonth;
		rec->u.year_by_day.month = MonthOfYear;
		break;

	case Week:
		rec->type = CRDF_TYPE_WEEK;
		rec->u.week.days = WeekDayRec2Proto(WeekDays);
		break;

	default:
		eout("RecurBase::BuildRecurrenceData: "
			"Unknown recurrence data type: 0x"
			<< setbase(16) << (unsigned int) rec->type);
		throw Error("RecurBase::BuildRecurrenceData: Unknown recurrence data type");
	}
}

uint8_t RecurBase::RecurringFieldType() const
{
	return FIELDCODE_RECURRENCE_DATA;
}

void RecurBase::Clear()
{
	Recurring = false;
	RecurringType = RecurBase::Week;
	Interval = 1;
	RecurringEndTime = 0;
	Perpetual = false;
	DayOfWeek = WeekOfMonth = DayOfMonth = MonthOfYear = 0;
	WeekDays = 0;
}

void RecurBase::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	static const char *DayNames[] = { "Sun", "Mon", "Tue", "Wed",
		"Thu", "Fri", "Sat" };
	static const char *MonthNames[] = { "Jan", "Feb", "Mar", "Apr",
		"May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// FIXME - need a "check all data" function that make sure that all
// recurrence data is within range.  Then call that before using
// the data, such as in Build and in Dump.

	// print recurrence data if available
	os << "   Recurring: " << (Recurring ? "yes" : "no") << "\n";
	if( Recurring ) {
		switch( RecurringType )
		{
		case Day:
			os << "      Every day.\n";
			break;

		case MonthByDate:
			os << "      Every month on the "
			   << DayOfMonth
			   << (DayOfMonth == 1 ? "st" : "")
			   << (DayOfMonth == 2 ? "nd" : "")
			   << (DayOfMonth == 3 ? "rd" : "")
			   << (DayOfMonth > 3  ? "th" : "")
			   << "\n";
			break;

		case MonthByDay:
			os << "      Every month on the "
			   << DayNames[DayOfWeek]
			   << " of week "
			   << WeekOfMonth
			   << "\n";
			break;

		case YearByDate:
			os << "      Every year on "
			   << MonthNames[MonthOfYear-1]
			   << " " << DayOfMonth << "\n";
			break;

		case YearByDay:
			os << "      Every year in " << MonthNames[MonthOfYear-1]
			   << " on "
			   << DayNames[DayOfWeek]
			   << " of week " << WeekOfMonth << "\n";
			break;

		case Week:
			os << "      Every week on: ";
			if( WeekDays & CAL_WD_SUN ) os << "Sun ";
			if( WeekDays & CAL_WD_MON ) os << "Mon ";
			if( WeekDays & CAL_WD_TUE ) os << "Tue ";
			if( WeekDays & CAL_WD_WED ) os << "Wed ";
			if( WeekDays & CAL_WD_THU ) os << "Thu ";
			if( WeekDays & CAL_WD_FRI ) os << "Fri ";
			if( WeekDays & CAL_WD_SAT ) os << "Sat ";
			os << "\n";
			break;

		default:
			os << "      Unknown recurrence type\n";
			break;
		}

		os << "      Interval: " << Interval << "\n";

		if( Perpetual )
			os << "      Ends: never\n";
		else
			os << "      Ends: " << ctime(&RecurringEndTime);
	}
}


} // namespace Barry

