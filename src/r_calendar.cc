///
/// \file	r_calendar.cc
///		Blackberry database record parser class for calendar records.
///

/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "r_calendar.h"
#include "record-internal.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "error.h"
#include "endian.h"
#include "iconv.h"
#include <ostream>
#include <iomanip>
#include <time.h>
#include <string.h>
#include <stdexcept>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {


///////////////////////////////////////////////////////////////////////////////
// Calendar class

// calendar field codes
#define CALFC_APPT_TYPE_FLAG		0x01
#define CALFC_SUBJECT			0x02
#define CALFC_NOTES			0x03
#define CALFC_LOCATION			0x04
#define CALFC_NOTIFICATION_TIME		0x05
#define CALFC_START_TIME		0x06
#define CALFC_END_TIME			0x07
#define CALFC_RECURRENCE_DATA		0x0c
#define CALFC_VERSION_DATA		0x10
#define CALFC_NOTIFICATION_DATA		0x1a
#define CALFC_FREEBUSY_FLAG		0x1c
#define CALFC_TIMEZONE_CODE		0x1e	// only seems to show up if recurring
#define CALFC_CLASS_FLAG		0x28    // private flag from outlook
#define CALFC_ALLDAYEVENT_FLAG		0xff
#define CALFC_END			0xffff

static FieldLink<Calendar> CalendarFieldLinks[] = {
   { CALFC_SUBJECT,    "Subject",    0, 0,    &Calendar::Subject, 0, 0, 0, 0, true },
   { CALFC_NOTES,      "Notes",      0, 0,    &Calendar::Notes, 0, 0, 0, 0, true },
   { CALFC_LOCATION,   "Location",   0, 0,    &Calendar::Location, 0, 0, 0, 0, true },
   { CALFC_NOTIFICATION_TIME,"Notification Time",0,0, 0, 0, &Calendar::NotificationTime, 0, 0, false },
   { CALFC_START_TIME, "Start Time", 0, 0,    0, 0, &Calendar::StartTime, 0, 0, false },
   { CALFC_END_TIME,   "End Time",   0, 0,    0, 0, &Calendar::EndTime, 0, 0, false },
   { CALFC_END,        "End of List",0, 0,    0, 0, 0, 0, 0, false }
};

Calendar::Calendar()
{
	Clear();
}

Calendar::~Calendar()
{
}

const unsigned char* Calendar::ParseField(const unsigned char *begin,
					  const unsigned char *end,
					  const IConverter *ic)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !btohs(field->size) )	// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	FieldLink<Calendar> *b = CalendarFieldLinks;
		b->type != CALFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				if( b->iconvNeeded && ic )
					s = ic->FromBB(s);
				return begin;	// done!
			}
			else if( b->timeMember && btohs(field->size) == 4 ) {
				time_t &t = this->*(b->timeMember);
				dout("min1900: " << field->u.min1900);
				t = min2time(field->u.min1900);
				return begin;
			}
		}
	}

	// handle special cases
	switch( field->type )
	{
	case CALFC_APPT_TYPE_FLAG:
		switch( field->u.raw[0] )
		{
		case 'a':			// regular non-recurring appointment
			Recurring = false;
			return begin;

		case '*':			// recurring appointment
			Recurring = true;
			return begin;

		default:
			throw Error("Calendar::ParseField: unknown appointment type");
		}
		break;

	case CALFC_ALLDAYEVENT_FLAG:
		AllDayEvent = field->u.raw[0] == 1;
		return begin;

	case CALFC_RECURRENCE_DATA:
		if( btohs(field->size) >= CALENDAR_RECURRENCE_DATA_FIELD_SIZE ) {
			// good data
			ParseRecurrenceData(&field->u.raw[0]);
		}
		else {
			// not enough data!
			throw Error("Calendar::ParseField: not enough data in recurrence data field");
		}
		return begin;

	case CALFC_TIMEZONE_CODE:
		if( btohs(field->size) == 2 ) {
			// good data
			TimeZoneCode = btohs(field->u.code);
		}
		else {
			throw Error("Calendar::ParseField: not enough data in time zone code field");
		}
		return begin;

	case CALFC_FREEBUSY_FLAG:
		FreeBusyFlag = (FreeBusyFlagType)field->u.raw[0];
		if( FreeBusyFlag > OutOfOffice ) {
			throw Error("Calendar::ParseField: FreeBusyFlag out of range" );
		}
		return begin;

	case CALFC_CLASS_FLAG:
		ClassFlag = (ClassFlagType)field->u.raw[0];
		if( ClassFlag > Private ) {
			throw Error("Calendar::ParseField: ClassFlag out of range" );
		}
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

// this function assumes the size has already been checked
void Calendar::ParseRecurrenceData(const void *data)
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

		// Note: this simple copy is only possible since
		// the CAL_WD_* constants are the same as CRDF_WD_* constants.
		// If this ever changes, this code will need to change.
		WeekDays = rec->u.week.days;
		break;

	default:
		eout("Unknown recurrence data type: " << rec->type);
		throw Error("Unknown recurrence data type");
	}
}

// this function assumes there is CALENDAR_RECURRENCE_DATA_FIELD_SIZE bytes
// available in data
void Calendar::BuildRecurrenceData(void *data) const
{
	if( !Recurring )
		throw Error("Calendar::BuildRecurrenceData: Attempting to build recurrence data on non-recurring record.");

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

		// Note: this simple copy is only possible since
		// the CAL_WD_* constants are the same as CRDF_WD_* constants.
		// If this ever changes, this code will need to change.
		rec->u.week.days = WeekDays;
		break;

	default:
		eout("Calendar::BuildRecurrenceData: "
			"Unknown recurrence data type: " << rec->type);
		throw Error("Calendar::BuildRecurrenceData: Unknown recurrence data type");
	}
}

void Calendar::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Calendar records
}

void Calendar::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void Calendar::BuildHeader(Data &data, size_t &offset) const
{
	// no header in Calendar records
}

//
// Build
//
/// Build fields part of record.
///
void Calendar::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	data.Zap();

	// output the type first
	BuildField(data, offset, CALFC_APPT_TYPE_FLAG, Recurring ? '*' : 'a');

	// output all day event flag only if set
	if( AllDayEvent )
		BuildField(data, offset, CALFC_ALLDAYEVENT_FLAG, (char)1);

	// cycle through the type table
	for(	const FieldLink<Calendar> *b = CalendarFieldLinks;
		b->type != CALFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				BuildField(data, offset, b->type, (b->iconvNeeded && ic) ? ic->ToBB(s) : s);
		}
		else if( b->timeMember ) {
			time_t t = this->*(b->timeMember);
			if( t > 0 )
				BuildField1900(data, offset, b->type, t);
		}
	}

	// handle special cases

	if( Recurring ) {
		CalendarRecurrenceDataField recur;
		BuildRecurrenceData(&recur);
		BuildField(data, offset, CALFC_RECURRENCE_DATA,
			&recur, CALENDAR_RECURRENCE_DATA_FIELD_SIZE);
	}

	if( TimeZoneValid )
		BuildField(data, offset, CALFC_TIMEZONE_CODE, TimeZoneCode);

	BuildField(data, offset, CALFC_FREEBUSY_FLAG, (char)FreeBusyFlag);
	BuildField(data, offset, CALFC_CLASS_FLAG, (char)ClassFlag);

	// and finally save unknowns
	UnknownsType::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	for( ; ub != ue; ub++ ) {
		BuildField(data, offset, *ub);
	}

	data.ReleaseBuffer(offset);
}

void Calendar::Clear()
{
	RecType = Calendar::GetDefaultRecType();

	AllDayEvent = false;
	Subject.clear();
	Notes.clear();
	Location.clear();
	NotificationTime = StartTime = EndTime = 0;

	FreeBusyFlag = Free;
	ClassFlag = Public;

	Recurring = false;
	RecurringType = Calendar::Week;
	Interval = 1;
	RecurringEndTime = 0;
	Perpetual = false;
	TimeZoneCode = GetTimeZoneCode(0, 0);	// default to GMT
	TimeZoneValid = false;
	DayOfWeek = WeekOfMonth = DayOfMonth = MonthOfYear = 0;
	WeekDays = 0;

	Unknowns.clear();
}

void Calendar::Dump(std::ostream &os) const
{
	static const char *DayNames[] = { "Sun", "Mon", "Tue", "Wed",
		"Thu", "Fri", "Sat" };
	static const char *MonthNames[] = { "Jan", "Feb", "Mar", "Apr",
		"May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	static const char *ClassTypes[] = { "Public", "Confidential", "Private" };
	static const char *FreeBusy[] = { "Free", "Tentative", "Busy", "Out of Office" };

// FIXME - need a "check all data" function that make sure that all
// recurrence data is within range.  Then call that before using
// the data, such as in Build and in Dump.

	os << "Calendar entry: 0x" << setbase(16) << RecordId
		<< " (" << (unsigned int)RecType << ")\n";
	os << "   All Day Event: " << (AllDayEvent ? "yes" : "no") << "\n";
	os << "   Class: " << ClassTypes[ClassFlag] << "\n";
	os << "   Free/Busy: " << FreeBusy[FreeBusyFlag] << "\n";
	if( TimeZoneValid )
		os << "   Time Zone: " << GetTimeZone(TimeZoneCode)->Name << "\n";

	// cycle through the type table
	for(	const FieldLink<Calendar> *b = CalendarFieldLinks;
		b->type != CALFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				os << "   " << b->name << ": " << s << "\n";
		}
		else if( b->timeMember ) {
			time_t t = this->*(b->timeMember);
			if( t > 0 )
				os << "   " << b->name << ": " << ctime(&t);
			else
				os << "   " << b->name << ": disabled\n";
		}
	}

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
			os << "      Ends: "
			   << ctime(&RecurringEndTime);
	}

	// print any unknowns
	os << Unknowns;
}


} // namespace Barry

