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
#include "iconv.h"
#include "debug.h"
#include <ostream>
#include <iomanip>
#include <string.h>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {


///////////////////////////////////////////////////////////////////////////////
// Task Class, static members

//
// Note! These functions currently only pass the same values through.
//       In actuality, these are technically two different values:
//       one on the raw protocol side, and the other part of the
//       guaranteed Barry API.  If the Blackberry ever changes the
//       meanings for these codes, do the translation here.
//

Task::AlarmFlagType Task::AlarmProto2Rec(uint8_t a)
{
	return (AlarmFlagType)a;
}

uint8_t Task::AlarmRec2Proto(AlarmFlagType a)
{
	return a;
}

Task::PriorityFlagType Task::PriorityProto2Rec(uint8_t p)
{
	return (PriorityFlagType)p;
}

uint8_t Task::PriorityRec2Proto(PriorityFlagType p)
{
	return p;
}

Task::StatusFlagType Task::StatusProto2Rec(uint8_t s)
{
	return (StatusFlagType)s;
}

uint8_t Task::StatusRec2Proto(StatusFlagType s)
{
	return s;
}


///////////////////////////////////////////////////////////////////////////////
// Task Class

// Task Field Codes
#define TSKFC_TASK_TYPE		0x01
#define TSKFC_TITLE		0x02
#define TSKFC_NOTES		0x03
#define TSKFC_DUE_TIME		0x05
#define TSKFC_START_TIME	0x06
#define TSKFC_DUE_FLAG		0x08
#define TSKFC_STATUS		0x09
#define TSKFC_PRIORITY		0x0a
#define TSKFC_ALARM_TYPE	0x0e
#define TSKFC_ALARM_TIME	0x0f
#define TSKFC_TIMEZONE_CODE	0x10
#define TSKFC_CATEGORIES	0x11
#define TSKFC_END		0xffff

static FieldLink<Task> TaskFieldLinks[] = {
   { TSKFC_TITLE,      "Summary",     0, 0, &Task::Summary, 0, 0, 0, 0, true },
   { TSKFC_NOTES,      "Notes",       0, 0, &Task::Notes, 0, 0, 0, 0, true },
   { TSKFC_START_TIME, "Start Time",  0, 0, 0, 0, &Task::StartTime, 0, 0, false },
   { TSKFC_DUE_TIME,   "Due Time",    0, 0, 0, 0, &Task::DueTime, 0, 0, false },
   { TSKFC_ALARM_TIME, "Alarm Time",  0, 0, 0, 0, &Task::AlarmTime, 0, 0, false },
   { TSKFC_CATEGORIES, "Categories",  0, 0, &Task::Categories, 0, 0, 0, 0, false },
   { TSKFC_END,        "End of List", 0, 0, 0, 0, 0, 0, 0, false },
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
				if( b->iconvNeeded && ic )
					s = ic->FromBB(s);
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
		if( field->u.raw[0] > TR_PRIORITY_RANGE_HIGH ) {
			throw Error( "Task::ParseField: priority field out of bounds" );
		}
		else {
			PriorityFlag = PriorityProto2Rec(field->u.raw[0]);
		}
		return begin;

	case TSKFC_STATUS:
		if( field->u.raw[0] > TR_STATUS_RANGE_HIGH ) {
			throw Error( "Task::ParseField: priority field out of bounds" );
		}
		else {
			StatusFlag = StatusProto2Rec(field->u.raw[0]);
		}
		return begin;

	case TSKFC_TIMEZONE_CODE:
		if( btohs(field->size) == 4 ) {
			TimeZoneCode = btohs(field->u.code);
			TimeZoneValid = true;
		}
		else {
			throw Error("Task::ParseField: not enough data in time zone code field");
		}
		return begin;

	case TSKFC_DUE_FLAG:
		DueDateFlag = field->u.raw[0];
		return begin;

	case TSKFC_ALARM_TYPE:
		if( field->u.raw[0] > TR_ALARM_RANGE_HIGH ) {
			throw Error("Task::ParseField: AlarmType out of bounds" );
		}
		else {
			AlarmType = AlarmProto2Rec(field->u.raw[0]);
		}
		return begin;
	}

	// base class handles recurring data
	if( RecurBase::ParseField(field->type, field->u.raw, btohs(field->size), ic) )
		return begin;

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
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


void Task::BuildHeader(Data &data, size_t &offset) const
{
	// no header in Task records
}


//
// Build
//
/// Build fields part of record.
///
void Task::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	data.Zap();

	// tack on the 't' task type field first
	BuildField(data, offset, TSKFC_TASK_TYPE, 't');

	// cycle through the type table
	for(	FieldLink<Task> *b = TaskFieldLinks;
		b->type != TSKFC_END;
		b++ )
	{
		// print only fields with data
		if( b->strMember ) {
			const std::string &field = this->*(b->strMember);
			if( field.size() ) {
				std::string s = (b->iconvNeeded && ic) ? ic->ToBB(field) : field;
				BuildField(data, offset, b->type, s);
			}
		}
		else if( b->timeMember ) {
			time_t t = this->*(b->timeMember);
			if( t > 0 )
				BuildField1900(data, offset, b->type, t);
		}
		else if( b->postMember && b->postField ) {
			const std::string &field = (this->*(b->postMember)).*(b->postField);
			if( field.size() ) {
				std::string s = (b->iconvNeeded && ic) ? ic->ToBB(field) : field;
				BuildField(data, offset, b->type, s);
			}
		}
	}

	BuildField(data, offset, TSKFC_STATUS, StatusRec2Proto(StatusFlag));
	BuildField(data, offset, TSKFC_PRIORITY, PriorityRec2Proto(PriorityFlag));
	BuildField(data, offset, TSKFC_ALARM_TYPE, AlarmRec2Proto(AlarmType));
	
	if ( DueDateFlag )
		BuildField(data, offset, TSKFC_DUE_FLAG, (char) 1);

	if( TimeZoneValid )
		BuildField(data, offset, TSKFC_TIMEZONE_CODE, TimeZoneCode);

	// and finally save unknowns
	UnknownsType::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	for( ; ub != ue; ub++ ) {
		BuildField(data, offset, *ub);
	}

	data.ReleaseBuffer(offset);
}



void Task::Clear()
{
	RecurBase::Clear();

	Summary.clear();
	Notes.clear();
	Categories.clear();
	StartTime = DueTime = AlarmTime = 0;

	PriorityFlag = (PriorityFlagType)0;
	StatusFlag = (StatusFlagType)0;
	AlarmType = (AlarmFlagType)0;

	TaskType = 0;

	DueDateFlag = false;

	TimeZoneCode = GetTimeZoneCode( 0, 0 );	// default to GMT
	TimeZoneValid = false;

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

	os << "   Due Date Flag: " << (DueDateFlag ? "true" : "false") << "\n";
	os << "   Priority: " << PriorityName[PriorityFlag] << "\n";
	os << "   Status: " << StatusName[StatusFlag] << "\n";
	if( AlarmType ) {
		os << "   Alarm Type: " << AlarmTypeName[AlarmType] << "\n";
	}
	if( TimeZoneValid )
		os << "   Time Zone: " << GetTimeZone(TimeZoneCode)->Name << "\n";

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

