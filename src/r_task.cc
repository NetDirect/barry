///
/// \file	r_task.cc
///		Record parsing class for the task database.
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2007, Brian Edginton

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

#include "r_task.h"
#include "r_calendar.h"                        // for CAL_* defines
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "debug.h"
#include <ostream>
#include <iomanip>
#include <string.h>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// Task Class

// Task Field Codes
#define TSKFC_TASK_TYPE		0x01
#define TSKFC_TITLE		0x02
#define TSKFC_NOTES		0x03
#define TSKFC_START_TIME	0x05
#define TSKFC_DUE_TIME		0x06
#define TSKFC_DUE_FLAG		0x08
#define TSKFC_STATUS		0x09
#define TSKFC_PRIORITY		0x0a
#define TSKFC_RECURRENCE_DATA	0x0c
#define TSKFC_ALARM_TYPE	0x0e
#define TSKFC_ALARM_TIME	0x0f
#define TSKFC_TIMEZONE_CODE	0x10
#define TSKFC_CATEGORIES	0x11
#define TSKFC_END		0xffff

static FieldLink<Task> TaskFieldLinks[] = {
	{ TSKFC_TITLE,      "Summary",     0, 0, &Task::Summary, 0, 0 },
	{ TSKFC_NOTES,      "Notes",       0, 0, &Task::Notes, 0, 0 },
	{ TSKFC_START_TIME, "Start Time",  0, 0, 0, 0, &Task::StartTime },
	{ TSKFC_DUE_TIME,   "Due Time",    0, 0, 0, 0, &Task::DueTime },
	{ TSKFC_ALARM_TIME, "Alarm Time",  0, 0, 0, 0, &Task::AlarmTime },
	{ TSKFC_CATEGORIES, "Categories",  0, 0, &Task::Categories, 0, 0 },
	{ TSKFC_END,        "End of List", 0, 0, 0, 0, 0 },
};

Task::Task()
{
	Clear();
}

Task::~Task()
{
}

const unsigned char* Task::ParseField(const unsigned char *begin,
				      const unsigned char *end,
				      const IConverter *ic)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if( begin > end )       // if begin==end, we are ok
		return begin;

	if( !btohs(field->size) )   // if field has no size, something's up
		return begin;

	if( field->type == TSKFC_TASK_TYPE ) {
		if( ( TaskType = field->u.raw[0] ) != 't' ) {
			throw Error("Task::ParseField: Task Type is not 't'");
		}
		return begin;
	}

	// cycle through the type table
	for(    FieldLink<Task> *b = TaskFieldLinks;
		b->type != TSKFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				return begin;   // done!
			}
			else if( b->timeMember && btohs(field->size) == 4 ) {
				time_t &t = this->*(b->timeMember);
				t = min2time(field->u.min1900);
				return begin;
			}
		}
	}
	// handle special cases
	switch( field->type )
	{
	case TSKFC_PRIORITY:
		if( field->u.raw[0] > Low ) {
			throw Error( "Task::ParseField: priority field out of bounds" );
		}
		else {
			PriorityFlag = (PriorityFlagType)field->u.raw[0];
		}
		return begin;

	case TSKFC_STATUS:
		if( field->u.raw[0] > Deferred ) {
			throw Error( "Task::ParseField: priority field out of bounds" );
		}
		else {
			StatusFlag = (StatusFlagType)field->u.raw[0];
		}
		return begin;

	case TSKFC_TIMEZONE_CODE:
		if( btohs(field->size) == 4 ) {
			TimeZoneCode = btohs(field->u.code);
		}
		else {
			throw Error("Task::ParseField: not enough data in time zone code field");
		}
		return begin;

	case TSKFC_RECURRENCE_DATA:
		if( btohs(field->size) >= CALENDAR_RECURRENCE_DATA_FIELD_SIZE ) {
			Recurring = true;
			ParseRecurrenceData(&field->u.raw[0]);
		}
		else {
			throw Error("Task::ParseField: not enough data in recurrence data field");
		}
		return begin;

	case TSKFC_DUE_FLAG:
		DueDateFlag = field->u.raw[0];
		return begin;		

	case TSKFC_ALARM_TYPE:
		if( field->u.raw[0] > Relative ) {
			throw Error("Task::ParseField: AlarmType out of bounds" );
		}
		else {
			AlarmType = (AlarmFlagType)field->u.raw[0];
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
void Task::ParseRecurrenceData(const void *data)
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
		eout("Unknown recurrence data type: 0x"
			<< setbase(16) << (unsigned int) rec->type);
		throw Error("Unknown recurrence data type");
	}
}

// this function assumes there is CALENDAR_RECURRENCE_DATA_FIELD_SIZE bytes
// available in data
void Task::BuildRecurrenceData(void *data)
{
	if( !Recurring )
		throw Error("Task::BuildRecurrenceData: Attempting to build recurrence data on non-recurring record.");

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
		eout("Task::BuildRecurrenceData: "
		"Unknown recurrence data type: " << rec->type);
		throw Error("Task::BuildRecurrenceData: Unknown recurrence data type");
	}
}

void Task::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Task records
}

void Task::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void Task::Clear()
{
	Summary.clear();
	Notes.clear();
	Categories.clear();
	StartTime = DueTime = AlarmTime = 0;

	PriorityFlag = (PriorityFlagType)0;
	StatusFlag = (StatusFlagType)0;
	AlarmType = (AlarmFlagType)0;

	TaskType = 0;

	Perpetual = false;
	DueDateFlag = false;
	Recurring = false;

	TimeZoneCode = GetTimeZoneCode( 0, 0 );

	Unknowns.clear();
}

void Task::Dump(std::ostream &os) const
{
	static const char *PriorityName[] = { "High", "Normal", "Low" };
	static const char *StatusName[] = { "Not Started", "In Progress",
		"Completed", "Waiting", "Deferred" };
	static const char *DayNames[] = { "Sun", "Mon", "Tue", "Wed",
		"Thu", "Fri", "Sat" };
	static const char *MonthNames[] = { "Jan", "Feb", "Mar", "Apr",
		"May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	static const char *AlarmTypeName[] = { "None", "By Date", "Relative" };

	os << "Task entry: 0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";

	// cycle through the type table
	for(	const FieldLink<Task> *b = TaskFieldLinks;
		b->type != TSKFC_END;
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
		}
	}

	os << "   Priority: " << PriorityName[PriorityFlag] << "\n";
	os << "   Status: " << StatusName[StatusFlag] << "\n";
	if( AlarmType ) {
		os << "   Alarm Type: " << AlarmTypeName[AlarmType] << "\n";
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
			os << "      Ends: " << ctime(&RecurringEndTime);
	}		

	os << Unknowns;
	os << "\n\n";
}

} // namespace Barry

