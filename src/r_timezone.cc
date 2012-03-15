///
/// \file	r_timezone.cc
///		Record parsing class for the timezone database.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2008, Brian Edginton

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

#include "r_timezone.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "iconv.h"
#include "debug.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "ios_state.h"
#include <algorithm>
#include "m_desktop.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry
{

///////////////////////////////////////////////////////////////////////////////
// TimeZone Class

// TimeZone Field Codes
#define TZFC_INDEX		0x01
#define TZFC_NAME		0x02
#define TZFC_OFFSET		0x03
#define TZFC_DST		0x04
#define TZFC_STARTMONTH		0x06
#define TZFC_ENDMONTH		0x0B
#define TZFC_TZTYPE		0x64

#define TZFC_END		0xffff

static FieldLink<TimeZone> TimeZoneFieldLinks[] = {
   { TZFC_NAME,   "Name",        0, 0, &TimeZone::Name, 0, 0, 0, 0, true },
   { TZFC_END,    "End of List", 0, 0, 0, 0, 0, 0, 0, false },
};

TimeZone::TimeZone()
{
	Clear();
}

TimeZone::TimeZone(int utc_offset)
{
	Clear();

	UTCOffset = utc_offset;
}

TimeZone::TimeZone(int hours, int minutes)
{
	Clear();

	// make sure that minutes are always positive, for our logic
	if( minutes < 0 )
		minutes = -minutes;

	// calculate offset in total minutes
	UTCOffset = hours * 60;
	if( hours < 0 )
		UTCOffset -= minutes;
	else
		UTCOffset += minutes;
}

TimeZone::~TimeZone()
{
}

const unsigned char* TimeZone::ParseField(const unsigned char *begin,
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

	if( field->type == TZFC_TZTYPE ) {
		if( ( TZType = field->u.uint32 ) != 1 ) {
			throw Error("TimeZone::ParseField: TimeZone Type is not valid");
		}
		return begin;
	}

	// cycle through the type table
	for(    FieldLink<TimeZone> *b = TimeZoneFieldLinks;
		b->type != TZFC_END;
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
		}
	}

	switch( field->type )
	{
	case TZFC_INDEX:
		Index = btohl(field->u.uint32);
		return begin;

	case TZFC_OFFSET:
		UTCOffset = btohs(field->u.int16);
		return begin;

	case TZFC_DST:
		DSTOffset = btohl(field->u.uint32);
		if (DSTOffset) {
			UseDST = true;
		}
		return begin;

	case TZFC_STARTMONTH:
		StartMonth = btohl(field->u.uint32);
		return begin;

	case TZFC_ENDMONTH:
		EndMonth = btohl(field->u.uint32);
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

void TimeZone::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Task records
}

void TimeZone::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void TimeZone::Validate() const
{
}

void TimeZone::BuildHeader(Data &data, size_t &offset) const
{
	// not yet implemented
}

void TimeZone::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	// not yet implemented
}

void TimeZone::Clear()
{
	RecType = GetDefaultRecType();
	RecordId = 0;

	Name.clear();
	Index = 0;
	UTCOffset = 0;

	UseDST = false;
	DSTOffset = 0;
	StartMonth = -1;
	EndMonth = -1;

	TZType = 0;

	Unknowns.clear();
}

const FieldHandle<TimeZone>::ListT& TimeZone::GetFieldHandles()
{
	static FieldHandle<TimeZone>::ListT fhv;

	if( fhv.size() )
		return fhv;

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME fhv

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME TimeZone

	FHP(RecType, "Record Type Code");
	FHP(RecordId, "Unique Record ID");

	FHD(Name, "TimeZone Name", TZFC_NAME, true);
	FHD(Index, "Index", TZFC_INDEX, false);
	FHD(UTCOffset, "TimeZone Offset in Minutes", TZFC_OFFSET, false);
	FHD(UseDST, "Use DST?", TZFC_DST, false);
	FHD(DSTOffset, "DST Offset", TZFC_DST, false);
	FHD(StartMonth, "Start Month", TZFC_STARTMONTH, false);
	FHD(EndMonth, "End Month", TZFC_ENDMONTH, false);
	FHD(TZType, "TimeZone Type", TZFC_TZTYPE, false);

	FHP(Unknowns, "Unknown Fields");

	return fhv;
}

std::string TimeZone::GetDescription() const
{
	ostringstream oss;
	oss << Name <<
		" (" << (UTCOffset > 0 ? "+" : "")
		<< dec << (UTCOffset / 60.0) << ")";
	return oss.str();
}

void TimeZone::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	static const char *month[] = {
			"Jan", "Feb", "Mar", "Apr", "May",
			"Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	os << "TimeZone entry: 0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";

	// cycle through the type table
	for(	const FieldLink<TimeZone> *b = TimeZoneFieldLinks;
		b->type != TZFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				os << "       " << b->name << ": " << s << "\n";
		}
	}

	int hours, minutes;
	Split(&hours, &minutes);

	os << "       Desc: " << GetDescription() << "\n";
	os << "      Index: 0x" <<setw(2) << Index << "\n";
	os << "       Type: 0x" <<setw(2) << (unsigned int)TZType << "\n";
	os << "     Offset: " << setbase(10) << UTCOffset << " minutes ("
		<< dec << (UTCOffset / 60.0) << ")\n";
	os << "            Split Offset: hours: "
		<< dec << hours << ", minutes: " << minutes << "\n";
	os << "  Sample TZ: " << GetTz("E") << "\n";
	os << "    Use DST: " << (UseDST ? "true" : "false") << "\n";
	if (UseDST) {
		os << " DST Offset: " << setbase(10) << DSTOffset << "\n";
		if ((StartMonth > 0) && (StartMonth < 11))
				os << "Start Month: " << month[StartMonth] << "\n";
		else
				os << "Start Month: unknown (" << setbase(10) << StartMonth << ")\n";
		if ((EndMonth > 0) && (EndMonth < 11))
			os << "  End Month: " << month[EndMonth] << "\n";
		else
			os << "  End Month: unknown (" << setbase(10) << EndMonth << ")\n";
	}

	os << Unknowns;
	os << "\n\n";
}


void TimeZone::Split(int *hours, int *minutes) const
{
	*hours = UTCOffset / 60;
	*minutes = UTCOffset % 60;
	if( *minutes < 0 )
		*minutes = -*minutes;
}

void TimeZone::SplitAbsolute(bool *west,
				unsigned int *hours,
				unsigned int *minutes) const
{
	int tmphours, tmpminutes;
	Split(&tmphours, &tmpminutes);

	if( tmphours < 0 ) {
		*west = true;
		tmphours = -tmphours;
	}
	else {
		*west = false;
	}

	*hours = tmphours;
	*minutes = tmpminutes;
}

std::string TimeZone::GetTz(const std::string &prefix) const
{
	int hours, minutes;
	Split(&hours, &minutes);

	// TZ variable uses positive for west, and negative for east,
	// so swap the hour value.  Minutes is always positive for
	// our purposes, so leave it alone.
	hours = -hours;

	// standard time first
	ostringstream oss;
	oss << prefix << "ST"
		<< hours << ":"
		<< setfill('0') << setw(2) << minutes
		<< ":00";

	// daylight saving time next, if available
	if( UseDST ) {
		TimeZone dst(UTCOffset + DSTOffset);

		dst.Split(&hours, &minutes);

		// swap again for TZ semantics...
		hours = -hours;

		oss << prefix << "DT"
			<< hours << ":"
			<< setfill('0') << setw(2) << minutes
			<< ":00";

		// assume second Sunday of start month to first Sunday of
		// end month, since we don't have enough data to be more
		// precise
		oss << ",M" << (StartMonth + 1) << ".2.0";
		oss << ",M" << (EndMonth + 1) << ".1.0";
	}

	return oss.str();
}


///////////////////////////////////////////////////////////////////////////////
// TimeZones Class

TimeZones::TimeZones()
{
	const StaticTimeZone *zones = GetStaticTimeZoneTable();
	for( ; zones->Name; zones++ ) {
		TimeZone tz(zones->HourOffset, zones->MinOffset);
		tz.Index = zones->Code;
		tz.Name = zones->Name;
		m_list.push_back(tz);
	}

	sort(begin(), end(), &TimeZone::SortByZone);
}

struct TimeZoneStore
{
	TimeZones::ListType &m_list;

	TimeZoneStore(TimeZones::ListType &list) : m_list(list)
	{
	}

	void operator()(const TimeZone &rec)
	{
		m_list.push_back(rec);
	}
};

TimeZones::TimeZones(Barry::Mode::Desktop &desktop)
{
	unsigned int dbId = desktop.GetDBID( TimeZone::GetDBName() );
	TimeZoneStore store(m_list);
	RecordParser<TimeZone, TimeZoneStore> parser(store);

	desktop.LoadDatabase(dbId, parser);

	sort(begin(), end(), &TimeZone::SortByZone);
}

bool TimeZones::IsLoadable(Barry::Mode::Desktop &desktop)
{
	unsigned int num;
	return desktop.GetDBDB().GetDBNumber(TimeZone::GetDBName(), num);
}

TimeZones::iterator TimeZones::Find(int index)
{
	for( iterator i = begin(), e = end(); i != e; ++i )
		if( i->Index == index )
			return i;
	return end();
}

TimeZones::const_iterator TimeZones::Find(int index) const
{
	return const_cast<TimeZones*>(this)->Find(index);
}

TimeZones::iterator TimeZones::FindByOffset(int utc_offset)
{
	for( iterator i = begin(), e = end(); i != e; ++i )
		if( i->UTCOffset == utc_offset )
			return i;
	return end();
}

TimeZones::const_iterator TimeZones::FindByOffset(int utc_offset) const
{
	return const_cast<TimeZones*>(this)->FindByOffset(utc_offset);
}

void TimeZones::Dump(std::ostream &os) const
{
	const_iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		os << *b << endl;
	}
}

} // namespace Barry

