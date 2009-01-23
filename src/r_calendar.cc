///
/// \file	r_calendar.cc
///		Blackberry database record parser class for calendar records.
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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
		BuildRecurrenceData(StartTime, &recur);
		BuildField(data, offset, RecurBase::RecurringFieldType(),
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
	RecurBase::Clear();

	RecType = Calendar::GetDefaultRecType();

	AllDayEvent = false;
	Subject.clear();
	Notes.clear();
	Location.clear();
	NotificationTime = StartTime = EndTime = 0;

	FreeBusyFlag = Free;
	ClassFlag = Public;

	TimeZoneCode = GetTimeZoneCode(0, 0);	// default to GMT
	TimeZoneValid = false;

	Unknowns.clear();
}

void Calendar::Dump(std::ostream &os) const
{
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
	RecurBase::Dump(os);

	// print any unknowns
	os << Unknowns;
}


} // namespace Barry

