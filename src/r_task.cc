///
/// \file	r_task.cc
///		Record parsing class for the task database.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
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

#include "i18n.h"
#include "r_task.h"
#include "r_calendar.h"                        // for CAL_* defines
#include "r_recur_base-int.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "iconv.h"
#include "debug.h"
#include <ostream>
#include <iomanip>
#include <string.h>
#include "ios_state.h"

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
#define TSKFC_START_TIME	0x06	// This is fuzzy... most devices seem
					// to anchor this value == to DUE_TIME
#define TSKFC_DUE_FLAG		0x08
#define TSKFC_STATUS		0x09
#define TSKFC_PRIORITY		0x0a
#define TSKFC_ALARM_TYPE	0x0e
#define TSKFC_ALARM_TIME	0x0f
#define TSKFC_TIMEZONE_CODE	0x10
#define TSKFC_CATEGORIES	0x11
#define TSKFC_ALARM_FLAG	0x12
#define TSKFC_END		0xffff

static FieldLink<Task> TaskFieldLinks[] = {
   { TSKFC_TITLE,      N_("Summary"),     0, 0, &Task::Summary, 0, 0, 0, 0, true },
   { TSKFC_NOTES,      N_("Notes"),       0, 0, &Task::Notes, 0, 0, 0, 0, true },
   { TSKFC_START_TIME, N_("Start Time"),  0, 0, 0, 0, &Task::StartTime, 0, 0, false },
   { TSKFC_DUE_TIME,   N_("Due Time"),    0, 0, 0, 0, &Task::DueTime, 0, 0, false },
   { TSKFC_ALARM_TIME, N_("Alarm Time"),  0, 0, 0, 0, &Task::AlarmTime, 0, 0, false },
   { TSKFC_END,        N_("End of List"), 0, 0, 0, 0, 0, 0, 0, false },
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
		if( field->u.raw[0] != 't' ) {
			throw Error(_("Task::ParseField: Task Type is not 't'"));
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
				TimeT &t = this->*(b->timeMember);
				t.Time = min2time(field->u.min1900);
				return begin;
			}
		}
	}

	// handle special cases
	switch( field->type )
	{
	case TSKFC_PRIORITY:
		if( field->u.raw[0] > TR_PRIORITY_RANGE_HIGH ) {
			throw Error(_("Task::ParseField: priority field out of bounds"));
		}
		else {
			PriorityFlag = PriorityProto2Rec(field->u.raw[0]);
		}
		return begin;

	case TSKFC_STATUS:
		if( field->u.raw[0] > TR_STATUS_RANGE_HIGH ) {
			throw Error(_("Task::ParseField: priority field out of bounds"));
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
			throw Error(_("Task::ParseField: not enough data in time zone code field"));
		}
		return begin;

	case TSKFC_DUE_FLAG:
		// the DueDateFlag is not available on really old devices
		// such as the 7750, and if the DueTime is available,
		// we'll just save it.  There is no further need for this
		// value that we know of yet, so just ignore it for now.
		return begin;

	case TSKFC_ALARM_FLAG:
		// the AlarmFlag is not available on really old devices
		// such as the 7750, and if the AlarmTime is available,
		// we'll just save it.  There is no further need for this
		// value that we know of yet, so just ignore it for now.
		return begin;

	case TSKFC_ALARM_TYPE:
		if( field->u.raw[0] > TR_ALARM_RANGE_HIGH ) {
			throw Error(_("Task::ParseField: AlarmType out of bounds"));
		}
		else {
			AlarmType = AlarmProto2Rec(field->u.raw[0]);
		}
		return begin;

	case TSKFC_CATEGORIES:
		{
			std::string catstring = ParseFieldString(field);
			if( ic )
				catstring = ic->FromBB(catstring);
			Categories.CategoryStr2List(catstring);
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


void Task::Validate() const
{
	RecurBase::Validate();
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

	//
	// Note: we do not use the FieldLink table, since the firmware
	// on many devices apears flakey, so we try to write as close to
	// the same order as we see coming from the device.  Unfortunately,
	// this doesn't really help, for the Tasks corruption bug, but
	// we'll keep the flexibility for now.
	//

	// tack on the 't' task type field first
	BuildField(data, offset, TSKFC_TASK_TYPE, 't');

	// Summary / Title
	if( Summary.size() ) {
		std::string s = (ic) ? ic->ToBB(Summary) : Summary;
		BuildField(data, offset, TSKFC_TITLE, s);
	}

	BuildField(data, offset, TSKFC_STATUS, (uint32_t) StatusRec2Proto(StatusFlag));
	BuildField(data, offset, TSKFC_PRIORITY, (uint32_t) PriorityRec2Proto(PriorityFlag));

	if( TimeZoneValid ) {
		// the time zone code field is 4 bytes, but we only use
		// the first two... pad it with zeros
		uint32_t code = TimeZoneCode;
		BuildField(data, offset, TSKFC_TIMEZONE_CODE, code);
	}

	// make sure StartTime matches DueTime, by writing it manually...
	/// not sure why StartTime exists, but oh well. :-)
	if( DueTime.IsValid() ) {
		// we use DueTime here, with the START_TIME code,
		// instead of StartTime, since the function is const
		BuildField1900(data, offset, TSKFC_START_TIME, DueTime);

		// then DueTime, with flag first, then time
		BuildField(data, offset, TSKFC_DUE_FLAG, (uint32_t) 1);
		BuildField1900(data, offset, TSKFC_DUE_TIME, DueTime);
	}

	if( AlarmTime.IsValid() ) {
		BuildField(data, offset, TSKFC_ALARM_FLAG, (uint32_t) 1);
		BuildField(data, offset, TSKFC_ALARM_TYPE, AlarmRec2Proto(AlarmType));
		BuildField1900(data, offset, TSKFC_ALARM_TIME, AlarmTime);
	}

	// Categories
	if( Categories.size() ) {
		string store;
		Categories.CategoryList2Str(store);
		BuildField(data, offset, TSKFC_CATEGORIES, ic ? ic->ToBB(store) : store);
	}

	// Notes
	if( Notes.size() ) {
		std::string s = (ic) ? ic->ToBB(Notes) : Notes;
		BuildField(data, offset, TSKFC_NOTES, s);
	}

	if( Recurring ) {
		CalendarRecurrenceDataField recur;
		BuildRecurrenceData(StartTime.Time, &recur);
		BuildField(data, offset, RecurBase::RecurringFieldType(),
			&recur, CALENDAR_RECURRENCE_DATA_FIELD_SIZE);
	}

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
	// clear the base class first
	RecurBase::Clear();

	// our variables...
	RecType = GetDefaultRecType();
	RecordId = 0;

	Summary.clear();
	Notes.clear();
	Categories.clear();
	UID.clear();

	StartTime.clear();
	DueTime.clear();
	AlarmTime.clear();

	TimeZoneCode = GetStaticTimeZoneCode( 0, 0 );	// default to GMT
	TimeZoneValid = false;

	AlarmType = Date;
	PriorityFlag = Normal;
	StatusFlag = NotStarted;

	Unknowns.clear();
}

const FieldHandle<Task>::ListT& Task::GetFieldHandles()
{
	static FieldHandle<Task>::ListT fhv;

	if( fhv.size() )
		return fhv;

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME fhv

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME Task

	FHP(RecType, _("Record Type Code"));
	FHP(RecordId, _("Unique Record ID"));

	FHD(Summary, _("Summary"), TSKFC_TITLE, true);
	FHD(Notes, _("Notes"), TSKFC_NOTES, true);
	FHD(Categories, _("Categories"), TSKFC_CATEGORIES, true);
	FHP(UID, _("UID"));	// FIXME - not linked to any device field??

	FHD(StartTime, _("Start Time"), TSKFC_START_TIME, false);
	FHD(DueTime, _("Due Time"), TSKFC_DUE_TIME, false);
	FHD(AlarmTime, _("Alarm Time"), TSKFC_ALARM_TIME, false);
	FHD(TimeZoneCode, _("Time Zone Code"), TSKFC_TIMEZONE_CODE, false);
	FHP(TimeZoneValid, _("Time Zone Code Valid"));

	FHE(aft, AlarmFlagType, AlarmType, _("Alarm Type"));
	FHE_CONST(aft, Date, _("Date"));
	FHE_CONST(aft, Relative, _("Relative"));

	FHE(pft, PriorityFlagType, PriorityFlag, _("Priority"));
	FHE_CONST(pft, High, _("High"));
	FHE_CONST(pft, Normal, _("Normal"));
	FHE_CONST(pft, Low, _("Low"));

	FHE(sft, StatusFlagType, StatusFlag, _("Status"));
	FHE_CONST(sft, NotStarted, _("Not Started"));
	FHE_CONST(sft, InProgress, _("In Progress"));
	FHE_CONST(sft, Completed, _("Completed"));
	FHE_CONST(sft, Waiting, _("Waiting"));
	FHE_CONST(sft, Deferred, _("Deferred"));

	FHP(Unknowns, _("Unknown Fields"));

	// and finally, the RecurBase fields
	RECUR_BASE_FIELD_HANDLES

	return fhv;
}

std::string Task::GetDescription() const
{
	return Summary;
}

void Task::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	static const char *PriorityName[] = {
		N_("High"),
		N_("Normal"),
		N_("Low")
	};
	static const char *StatusName[] = {
		N_("Not Started"),
		N_("In Progress"),
		N_("Completed"),
		N_("Waiting"),
		N_("Deferred")
	};
	static const char *AlarmTypeName[] = {
		N_("None"),
		N_("By Date"),
		N_("Relative")
	};

	os << _("Task entry: ") << "0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";

	// cycle through the type table
	for(	const FieldLink<Task> *b = TaskFieldLinks;
		b->type != TSKFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				os << "   " << gettext(b->name) << ": " << Cr2LfWrapper(s) << "\n";
		}
		else if( b->timeMember ) {
			TimeT t = this->*(b->timeMember);
			if( t.IsValid() )
				os << "   " << gettext(b->name) << ": " << t << "\n";
		}
	}

	os << _("   Priority: ") << gettext(PriorityName[PriorityFlag]) << "\n";
	os << _("   Status: ") << gettext(StatusName[StatusFlag]) << "\n";
	if( AlarmType ) {
		os << _("   Alarm Type: ") << gettext(AlarmTypeName[AlarmType]) << "\n";
	}
	if( TimeZoneValid )
		os << _("   Time Zone: ") << gettext(GetStaticTimeZone(TimeZoneCode)->Name) << "\n";

	// print recurrence data if available
	RecurBase::Dump(os);

	if( Categories.size() ) {
		string display;
		Categories.CategoryList2Str(display);
		os << _("   Categories: ") << display << "\n";
	}

	os << Unknowns;
	os << "\n\n";
}

bool Task::operator<(const Task &other) const
{
	if( StartTime != other.StartTime )
		return StartTime < other.StartTime;
	if( AlarmTime != other.AlarmTime )
		return AlarmTime < other.AlarmTime;

	int cmp = Summary.compare(other.Summary);
	if( cmp == 0 )
		cmp = Notes.compare(other.Notes);
	return cmp < 0;
}

} // namespace Barry

