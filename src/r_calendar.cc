///
/// \file	r_calendar.cc
///		Blackberry database record parser class for calendar records.
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
#include "r_calendar.h"
#include "r_recur_base-int.h"
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
#include "ios_state.h"

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {


///////////////////////////////////////////////////////////////////////////////
// Calendar class, static members

//
// Note! These functions currently only pass the same values through.
//       In actuality, these are technically two different values:
//       one on the raw protocol side, and the other part of the
//       guaranteed Barry API.  If the Blackberry ever changes the
//       meanings for these codes, do the translation here.
//

Calendar::FreeBusyFlagType Calendar::FreeBusyFlagProto2Rec(uint8_t f)
{
	return (FreeBusyFlagType)f;
}

uint8_t Calendar::FreeBusyFlagRec2Proto(FreeBusyFlagType f)
{
	return f;
}

Calendar::ClassFlagType Calendar::ClassFlagProto2Rec(uint8_t f)
{
	return (ClassFlagType)f;
}

uint8_t Calendar::ClassFlagRec2Proto(ClassFlagType f)
{
	return f;
}



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
#define CALFC_ACCEPTED_BY		0x0b
#define CALFC_VERSION_DATA		0x10
#define CALFC_INVITED			0x15
#define CALFC_ORGANIZER			0x16
#define CALFC_NOTIFICATION_DATA		0x1a
#define CALFC_FREEBUSY_FLAG		0x1c
#define CALFC_TIMEZONE_CODE		0x1e	// only seems to show up if recurring
#define CALFC_CLASS_FLAG		0x28    // private flag from outlook
#define CALFC_CALENDAR_ID		0x2b	// Calendar using (new devices have several calendar)
#define CALFC_ALLDAYEVENT_FLAG		0xff
#define CALFC_END			0xffff

static FieldLink<Calendar> CalendarFieldLinks[] = {
   { CALFC_SUBJECT,    N_("Subject"),    0, 0,    &Calendar::Subject, 0, 0, 0, 0, true },
   { CALFC_NOTES,      N_("Notes"),      0, 0,    &Calendar::Notes, 0, 0, 0, 0, true },
   { CALFC_LOCATION,   N_("Location"),   0, 0,    &Calendar::Location, 0, 0, 0, 0, true },
   { CALFC_NOTIFICATION_TIME,N_("Notification Time"),0,0, 0, 0, &Calendar::NotificationTime, 0, 0, false },
   { CALFC_START_TIME, N_("Start Time"), 0, 0,    0, 0, &Calendar::StartTime, 0, 0, false },
   { CALFC_END_TIME,   N_("End Time"),   0, 0,    0, 0, &Calendar::EndTime, 0, 0, false },
   { CALFC_ORGANIZER,  N_("Organizer"),  0, 0,    0, &Calendar::Organizer, 0, 0, 0, true },
   { CALFC_ACCEPTED_BY,N_("Accepted By"),0, 0,    0, &Calendar::AcceptedBy, 0, 0, 0, true },
   { CALFC_INVITED,    N_("Invited"),    0, 0,    0, &Calendar::Invited, 0, 0, 0, true },
   { CALFC_END,        N_("End of List"),0, 0,    0, 0, 0, 0, 0, false }
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
				TimeT &t = this->*(b->timeMember);
				dout("min1900: " << field->u.min1900);
				t.Time = min2time(field->u.min1900);
				return begin;
			}
			else if( b->addrMember ) {
				//
				// parse email address
				// get dual addr+name string first
				// Note: this is a different format than
				// used in r_message*.cc
				//
				std::string dual((const char*)field->u.raw, btohs(field->size));

				EmailAddress a;

				// assign first string, using null terminator
				// letting std::string add it for us if it
				// doesn't exist
				a.Email = dual.c_str();

				// assign second string, using first size
				// as starting point
				a.Name = dual.c_str() + a.Email.size() + 1;

				// if the address is non-empty, add to list
				if( a.size() ) {
					// i18n convert if needed
					if( b->iconvNeeded && ic ) {
						a.Name = ic->FromBB(a.Name);
						a.Email = ic->FromBB(a.Email);
					}

					EmailAddressList &al = this->*(b->addrMember);
					al.push_back(a);
				}

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
			throw Error(_("Calendar::ParseField: unknown appointment type"));
		}
		break;

	case CALFC_ALLDAYEVENT_FLAG:
		AllDayEvent = field->u.raw[0] == 1;
		return begin;

	case CALFC_TIMEZONE_CODE:
		if( btohs(field->size) == 2 ) {
			// good data
			TimeZoneCode = btohs(field->u.code);
			TimeZoneValid = true;
		}
		else {
			throw Error(_("Calendar::ParseField: not enough data in time zone code field"));
		}
		return begin;

	case CALFC_FREEBUSY_FLAG:
		if( field->u.raw[0] > CR_FREEBUSY_RANGE_HIGH ) {
			throw Error(_("Calendar::ParseField: FreeBusyFlag out of range"));
		}
		FreeBusyFlag = FreeBusyFlagProto2Rec(field->u.raw[0]);
		return begin;

	case CALFC_CALENDAR_ID:
		if( btohs(field->size) == 8 ) {
			CalendarID = btohll(field->u.uint64);
		}
		else {
			throw Error(_("Calendar::ParseField: size data unknown in calendar field"));
		}
		return begin;

	case CALFC_CLASS_FLAG:
		if( field->u.raw[0] > CR_CLASS_RANGE_HIGH ) {
			throw Error(_("Calendar::ParseField: ClassFlag out of range" ));
		}
		ClassFlag = ClassFlagProto2Rec(field->u.raw[0]);
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

void Calendar::Validate() const
{
	RecurBase::Validate();
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
			TimeT t = this->*(b->timeMember);
			if( t.Time > 0 )
				BuildField1900(data, offset, b->type, t.Time);
		}
		else if( b->addrMember ) {
			const EmailAddressList &al = this->*(b->addrMember);
			EmailAddressList::const_iterator lb = al.begin(), le = al.end();

			// add all entries in list
			for( ; lb != le; ++lb ) {

				// skip empty entries
				if( !lb->size() )
					continue;

				std::string Name = lb->Name,
					Email = lb->Email;

				// do i18n conversion only if needed
				if( b->iconvNeeded && ic ) {
					Name = ic->ToBB(Name);
					Email = ic->ToBB(Email);
				}

				//
				// Build an addr+name field, each string
				// null terminated.
				// Note: this is a different format than
				// what is used in r_message*.cc
				//
				std::string field(lb->Email.c_str(), lb->Email.size() + 1);
				field.append(lb->Name.c_str(), lb->Name.size() + 1);
				BuildField(data, offset, b->type, field.data(), field.size());
			}
		}
	}

	// handle special cases
	if( Recurring ) {
		CalendarRecurrenceDataField recur;
		BuildRecurrenceData(StartTime.Time, &recur);
		BuildField(data, offset, RecurBase::RecurringFieldType(),
			&recur, CALENDAR_RECURRENCE_DATA_FIELD_SIZE);
	}

	if( TimeZoneValid )
		BuildField(data, offset, CALFC_TIMEZONE_CODE, TimeZoneCode);

	BuildField(data, offset, CALFC_FREEBUSY_FLAG, FreeBusyFlagRec2Proto(FreeBusyFlag));
	BuildField(data, offset, CALFC_CLASS_FLAG, ClassFlagRec2Proto(ClassFlag));

	// If CalendarID is defined and most of supported !
	// (by default 0xffff ffff ffff ffff)
	if( CalendarID != (uint64_t) -1 )
		BuildField(data, offset, CALFC_CALENDAR_ID, CalendarID);

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
	// clear the base class too
	RecurBase::Clear();

	// clear our fields
	RecType = GetDefaultRecType();
	RecordId = 0;

	AllDayEvent = false;
	Subject.clear();
	Notes.clear();
	Location.clear();
	NotificationTime.clear();
	StartTime.clear();
	EndTime.clear();
	Organizer.clear();
	AcceptedBy.clear();
	Invited.clear();

	FreeBusyFlag = Free;
	ClassFlag = Public;

	CalendarID = btohll((uint64_t) -1);

	TimeZoneCode = GetStaticTimeZoneCode(0, 0);	// default to GMT
	TimeZoneValid = false;

	Unknowns.clear();
}

const FieldHandle<Calendar>::ListT& Calendar::GetFieldHandles()
{
	static FieldHandle<Calendar>::ListT fhv;

	if( fhv.size() )
		return fhv;

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME fhv

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME Calendar

#define ALL_COMMON_CALENDAR_FIELDS \
	FHP(RecType, _("Record Type Code")); \
	FHP(RecordId, _("Unique Record ID")); \
 \
	FHP(AllDayEvent, _("All Day Event")); \
	FHD(Subject, _("Subject"), CALFC_SUBJECT, true); \
	FHD(Notes, _("Notes"), CALFC_NOTES, true); \
	FHD(Location, _("Location"), CALFC_LOCATION, true); \
	FHD(NotificationTime, _("Notification Time (0 is off)"), \
				CALFC_NOTIFICATION_TIME, false); \
	FHD(StartTime, _("Start Time"), CALFC_START_TIME, false); \
	FHD(EndTime, _("End Time"), CALFC_END_TIME, false); \
	FHD(Organizer, _("Organizer"), CALFC_ORGANIZER, true); \
	FHD(AcceptedBy, _("Accepted By"), CALFC_ACCEPTED_BY, true); \
	FHD(Invited, _("Invited"), CALFC_INVITED, true); \
 \
	FHE(fbf, FreeBusyFlagType, FreeBusyFlag, _("Free or Busy Flag")); \
	FHE_CONST(fbf, Free, _("Free")); \
	FHE_CONST(fbf, Tentative, _("Tentative")); \
	FHE_CONST(fbf, Busy, _("Busy")); \
	FHE_CONST(fbf, OutOfOffice, _("Out of Office")); \
 \
	FHE(cf, ClassFlagType, ClassFlag, _("Event Class")); \
	FHE_CONST(cf, Public, _("Public")); \
	FHE_CONST(cf, Confidential, _("Confidential")); \
	FHE_CONST(cf, Private, _("Private")); \
 \
	FHP(TimeZoneCode, _("Time Zone Code")); \
	FHP(TimeZoneValid, _("Time Zone Validity")); \
 \
	FHP(Unknowns, _("Unknown Fields"));

	ALL_COMMON_CALENDAR_FIELDS

	// the fields unique to Calendar, or different in CalendarALL
	FHD(CalendarID, _("Calendar ID"), CALFC_CALENDAR_ID, false);

	// and finally, the RecurBase fields
	RECUR_BASE_FIELD_HANDLES

	return fhv;
}

std::string Calendar::GetDescription() const
{
	return Subject;
}

void Calendar::DumpSpecialFields(std::ostream &os) const
{
	ios_format_state state(os);

	static const char *ClassTypes[] = {
		N_("Public"),
		N_("Confidential"),
		N_("Private")
	};
	static const char *FreeBusy[] = {
		N_("Free"),
		N_("Tentative"),
		N_("Busy"),
		N_("Out of Office")
	};

	os << _("   Calendar ID: ")
		<< "0x" << setbase(16) << CalendarID << "\n";
	os << _("   All Day Event: ") << (AllDayEvent ? "yes" : "no") << "\n";
	os << _("   Class: ") << ClassTypes[ClassFlag] << "\n";
	os << _("   Free/Busy: ") << FreeBusy[FreeBusyFlag] << "\n";
	if( TimeZoneValid )
		os << _("   Time Zone: ") << gettext(GetStaticTimeZone(TimeZoneCode)->Name) << "\n";
}

void Calendar::Dump(std::ostream &os) const
{
	ios_format_state state(os);

// FIXME - need a "check all data" function that make sure that all
// recurrence data is within range.  Then call that before using
// the data, such as in Build and in Dump.

	os << _("Calendar entry: ") << "0x" << setbase(16) << RecordId
		<< " (" << (unsigned int)RecType << ")\n";
	DumpSpecialFields(os);

	// cycle through the type table
	for(	const FieldLink<Calendar> *b = CalendarFieldLinks;
		b->type != CALFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				os << "   " << gettext(b->name) << ": " << s << "\n";
		}
		else if( b->timeMember ) {
			TimeT t = this->*(b->timeMember);
			if( t.Time > 0 )
				os << "   " << gettext(b->name) << ": " << t << "\n";
			else
				os << "   " << gettext(b->name) << ": " << _("disabled") << "\n";
		}
		else if( b->addrMember ) {
			const EmailAddressList &al = this->*(b->addrMember);
			EmailAddressList::const_iterator lb = al.begin(), le = al.end();

			for( ; lb != le; ++lb ) {
				if( !lb->size() )
					continue;

				os << "   " << gettext(b->name) << ": " << *lb << "\n";
			}
		}
	}

	// print recurrence data if available
	RecurBase::Dump(os);

	// print any unknowns
	os << Unknowns;
}

bool Calendar::operator<(const Calendar &other) const
{
	if( StartTime < other.StartTime )
		return true;
	else if( other.StartTime < StartTime )
		return false;

	// times are equal, so secondary sort based on Subject + Location
	int cmp = Subject.compare(other.Subject);
	if( cmp == 0 )
		cmp = Location.compare(other.Location);
	return cmp < 0;
}


///////////////////////////////////////////////////////////////////////////////
// Calendar-All class

// calendar-all field codes
#define CALALLFC_CALENDAR_ID		0x02	// Calendar using (new devices have several calendar)
#define CALALLFC_MAIL_ACCOUNT		0x03
#define CALALLFC_UNIQUEID			0x05
#define CALALLFC_CAL_OBJECT			0x0a
#define CALALLFC_END				0xffff

void CalendarAll::Clear()
{
	Calendar::Clear();

	MailAccount.clear();
}

const FieldHandle<CalendarAll>::ListT& CalendarAll::GetFieldHandles()
{
	static FieldHandle<CalendarAll>::ListT fhv;

	if( fhv.size() )
		return fhv;

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME fhv

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME CalendarAll

	ALL_COMMON_CALENDAR_FIELDS

	// Calendar:: field, but with a CalendarAll ID
	FHD(CalendarID, _("Calendar ID"), CALALLFC_CALENDAR_ID, false);

	// add the fields specific to CalendarAll
	FHD(MailAccount, _("Mail Account"), CALALLFC_MAIL_ACCOUNT, true);

	// and finally, the RecurBase fields
	RECUR_BASE_FIELD_HANDLES

	return fhv;
}

void CalendarAll::ParseHeader(const Data &data, size_t &offset)
{
	const unsigned char *b = (const unsigned char*) (data.GetData() + offset);
	const unsigned char *e = (const unsigned char*) (data.GetData() + data.GetSize());

	while( (b + COMMON_FIELD_HEADER_SIZE) < e ) {
		const CommonField *field = (const CommonField *) b;

		// advance and check size
		b += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
		if( b > e )					// if begin==end, we are ok
			continue;

		if( !btohs(field->size) )	// if field has no size, something's up
			continue;

		// handle special cases
		if( field->type == CALALLFC_CAL_OBJECT )
		{
			b -= btohs(field->size);
			// end of header
			break;
		}

		switch( field->type )
		{
		case CALALLFC_CALENDAR_ID:
			if( btohs(field->size) == 8 ) {
				CalendarID = btohll(field->u.uint64);
			}
			else {
				throw Error(_("CalendarAll::ParseField: size data unknown in calendar field"));
			}
			continue;

		case CALALLFC_MAIL_ACCOUNT:
			MailAccount = ParseFieldString(field);
			continue;

		case CALALLFC_UNIQUEID:
			if( btohs(field->size) == 4 ) {
				RecordId = btohl(field->u.uint32);
			}
			else {
				throw Error(_("CalendarAll::ParseHeader: size data unknown in calendar field"));
			}
			continue;
		}

		// if still not handled, add to the Unknowns list
		UnknownField uf;
		uf.type = field->type;
		uf.data.assign((const char*)field->u.raw, btohs(field->size));
		Unknowns.push_back(uf);
	}

	offset += b - (data.GetData() + offset);
}

void CalendarAll::DumpSpecialFields(std::ostream &os) const
{
	ios_format_state state(os);

	Calendar::DumpSpecialFields(os);
	os << _("   Mail Account: ") << MailAccount << "\n";
}

} // namespace Barry

