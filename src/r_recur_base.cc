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

#include "i18n.h"
#include "r_recur_base.h"
#include "protostructs.h"
#include "error.h"
#include "endian.h"
#include "time.h"
#include "ios_state.h"
#include <string.h>
#include <iomanip>

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
			throw Error(_("RecurBase::ParseField: not enough data in recurrence data field"));
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
		RecurringEndTime.Time = min2time(rec->endTime);
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
		throw Error(_("Unknown recurrence data type"));
	}

	Recurring = true;
}

void RecurBase::Validate() const
{
}

// this function assumes there is CALENDAR_RECURRENCE_DATA_FIELD_SIZE bytes
// available in data
void RecurBase::BuildRecurrenceData(time_t StartTime, void *data) const
{
	if( !Recurring )
		throw Error(_("RecurBase::BuildRecurrenceData: Attempting to build recurrence data on non-recurring record."));

	CalendarRecurrenceDataField *rec = (CalendarRecurrenceDataField*) data;

	// set all to zero
	memset(data, 0, CALENDAR_RECURRENCE_DATA_FIELD_SIZE);

	rec->interval = htobs(Interval);
	rec->startTime = time2min(StartTime);
	if( Perpetual )
		rec->endTime = 0xffffffff;
	else
		rec->endTime = time2min(RecurringEndTime.Time);

	switch( RecurringType )
	{
	case Day:
		rec->type = CRDF_TYPE_DAY;
		// no extra data
		break;

	case MonthByDate:
		rec->type = CRDF_TYPE_MONTH_BY_DATE;
		rec->u.month_by_date.monthDay = (uint8_t)DayOfMonth;
		break;

	case MonthByDay:
		rec->type = CRDF_TYPE_MONTH_BY_DAY;
		rec->u.month_by_day.weekDay = (uint8_t)DayOfWeek;
		rec->u.month_by_day.week = (uint8_t)WeekOfMonth;
		break;

	case YearByDate:
		rec->type = CRDF_TYPE_YEAR_BY_DATE;
		rec->u.year_by_date.monthDay = (uint8_t)DayOfMonth;
		rec->u.year_by_date.month = (uint8_t)MonthOfYear;
		break;

	case YearByDay:
		rec->type = CRDF_TYPE_YEAR_BY_DAY;
		rec->u.year_by_day.weekDay = (uint8_t)DayOfWeek;
		rec->u.year_by_day.week = (uint8_t)WeekOfMonth;
		rec->u.year_by_day.month = (uint8_t)MonthOfYear;
		break;

	case Week:
		rec->type = CRDF_TYPE_WEEK;
		rec->u.week.days = WeekDayRec2Proto(WeekDays);
		break;

	default:
		eout("RecurBase::BuildRecurrenceData: "
			"Unknown recurrence data type: 0x"
			<< setbase(16) << (unsigned int) rec->type);
		throw Error(_("RecurBase::BuildRecurrenceData: Unknown recurrence data type"));
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
	RecurringEndTime.clear();
	Perpetual = false;
	DayOfWeek = WeekOfMonth = DayOfMonth = MonthOfYear = 0;
	WeekDays = 0;
}

void RecurBase::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	static const char *DayNames[] = {
		N_("Sun"),
		N_("Mon"),
		N_("Tue"),
		N_("Wed"),
		N_("Thu"),
		N_("Fri"),
		N_("Sat")
	};
	static const char *MonthNames[] = {
		N_("Jan"),
		N_("Feb"),
		N_("Mar"),
		N_("Apr"),
		N_("May"),
		N_("Jun"),
		N_("Jul"),
		N_("Aug"),
		N_("Sep"),
		N_("Oct"),
		N_("Nov"),
		N_("Dec")
	};

// FIXME - need a "check all data" function that make sure that all
// recurrence data is within range.  Then call that before using
// the data, such as in Build and in Dump.

	// print recurrence data if available
	os << _("   Recurring: ") << (Recurring ? _("yes") : _("no")) << "\n";
	if( Recurring ) {
		switch( RecurringType )
		{
		case Day:
			os << _("      Every day.\n");
			break;

		case MonthByDate:
			// TRANSLATORS: to remove the 'th' ending on numbers,
			// just skip the %s in this string.
			os << string_vprintf(_("      Every month on the %u%s"),
				DayOfMonth,
				(DayOfMonth == 1 ? "st" :
					(DayOfMonth == 2 ? "nd" :
					(DayOfMonth == 3 ? "rd" :
					(DayOfMonth > 3  ? "th" : "")))))
			   << "\n";
			break;

		case MonthByDay:
			os << string_vprintf(_("      Every month on the %s of week %u"),
				gettext( DayNames[DayOfWeek] ),
				WeekOfMonth)
			   << "\n";
			break;

		case YearByDate:
			os << string_vprintf(_("      Every year on %s %u"),
				gettext( MonthNames[MonthOfYear-1] ),
				DayOfMonth)
			   << "\n";
			break;

		case YearByDay:
			os << string_vprintf(_("      Every year in %s on %s of week %u"),
				gettext( MonthNames[MonthOfYear-1] ),
				gettext( DayNames[DayOfWeek] ),
				WeekOfMonth)
			   << "\n";
			break;

		case Week:
			os << _("      Every week on: ");
			if( WeekDays & CAL_WD_SUN ) os << gettext("Sun") << " ";
			if( WeekDays & CAL_WD_MON ) os << gettext("Mon") << " ";
			if( WeekDays & CAL_WD_TUE ) os << gettext("Tue") << " ";
			if( WeekDays & CAL_WD_WED ) os << gettext("Wed") << " ";
			if( WeekDays & CAL_WD_THU ) os << gettext("Thu") << " ";
			if( WeekDays & CAL_WD_FRI ) os << gettext("Fri") << " ";
			if( WeekDays & CAL_WD_SAT ) os << gettext("Sat") << " ";
			os << "\n";
			break;

		default:
			os << _("      Unknown recurrence type\n");
			break;
		}

		os << dec << _("      Interval: ") << Interval << "\n";

		if( Perpetual )
			os << _("      Ends: never\n");
		else
			os << _("      Ends: ") << RecurringEndTime << "\n";
	}
}


} // namespace Barry

