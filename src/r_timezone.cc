///
/// \file	r_timezone.cc
///		Record parsing class for the timezone database.
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
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
#include <ostream>
#include <iomanip>

using namespace std;
using namespace Barry::Protocol;

namespace Barry
{

///////////////////////////////////////////////////////////////////////////////
// Timezone Class

// Timezone Field Codes
#define TZFC_INDEX		0x01
#define TZFC_NAME		0x02
#define TZFC_OFFSET		0x03
#define TZFC_DST		0x04
#define TZFC_STARTMONTH		0x06
#define TZFC_ENDMONTH		0x0B
#define TZFC_TZTYPE		0x64

#define TZFC_END		0xffff

static FieldLink<Timezone> TimezoneFieldLinks[] = {
   { TZFC_NAME,   "Name",        0, 0, &Timezone::TimeZoneName, 0, 0, 0, 0, true },
   { TZFC_END,    "End of List", 0, 0, 0, 0, 0, 0, 0, false },
};

Timezone::Timezone()
{
	Clear();
}

Timezone::~Timezone()
{
}

const unsigned char* Timezone::ParseField(const unsigned char *begin,
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
			throw Error("Timezone::ParseField: Timezone Type is not valid");
		}
		return begin;
	}

	// cycle through the type table
	for(    FieldLink<Timezone> *b = TimezoneFieldLinks;
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
		Offset = btohs(field->u.int16);
		if (Offset < 0) {
			Offset =~ Offset;
			Offset++;
			OffsetFraction = Offset % 60;
			Offset = Offset / 60;
			Left = true;
		} else {
			OffsetFraction = Offset % 60;
			Offset = Offset / 60;
			Left = false;
		}
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

void Timezone::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Task records
}

void Timezone::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void Timezone::Clear()
{
	TimeZoneName.clear();
	
	Index = 0;
	Left = false;
	UseDST = false;
	Offset = 0;
	OffsetFraction = 0;
	DSTOffset = 0;
	StartMonth = -1;
	EndMonth = -1;

	Unknowns.clear();
}

void Timezone::Dump(std::ostream &os) const
{
	static const char *month[] = {
			"Jan", "Feb", "Mar", "Apr", "May",
			"Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
	};
	
	os << "Task entry: 0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";

	// cycle through the type table
	for(	const FieldLink<Timezone> *b = TimezoneFieldLinks;
		b->type != TZFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				os << "       " << b->name << ": " << s << "\n";
		}
	}

	os << "      Index: 0x" <<setw(2) << Index << "\n";
	os << "     Offset: " << (Left ? "-" : "+") << setbase(10) << Offset << "." << OffsetFraction << "\n";
	os << "    Use DST: " << (UseDST ? "true" : "false") << "\n";
	if (UseDST) {
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

}
