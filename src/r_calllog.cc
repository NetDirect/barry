///
/// \file	r_calllog.cc
///		Record parsing class for the phone call logs database.
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
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

#include "r_calllog.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "iconv.h"
#include <ostream>
#include <iomanip>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

CallLog::StatusFlagType CallLog::StatusProto2Rec(uint8_t s)
{
	return (StatusFlagType)s;
}

uint8_t CallLog::StatusRec2Proto(StatusFlagType s)
{
	return s;
}


///////////////////////////////////////////////////////////////////////////////
// CallLog Class

// CallLog Field Codes
#define CLLFC_CALLLOG_TYPE		0x01
#define CLLFC_STATUS			0x02
#define CLLFC_PHONE				0x0c
#define CLLFC_NAME				0x1f
#define CLLFC_TIME				0x05
#define CLLFC_UNIQUEID			0x07
#define CLLFC_END				0xffff

static FieldLink<CallLog> CallLogFieldLinks[] = {
    { CLLFC_PHONE,     "Phone number",  0, 0, &CallLog::Phone, 0, 0, 0, 0, true },
    { CLLFC_NAME,      "Contact name",  0, 0, &CallLog::Name, 0, 0, 0, 0, true },
//	{ CLLFC_TIME,      "Time",          0, 0, 0, 0, &CallLog::Time, 0, 0, false },
    { CLLFC_END,       "End of List",   0, 0, 0, 0, 0, 0, 0, false }
};

CallLog::CallLog()
{
	Clear();
}

CallLog::~CallLog()
{
}

const unsigned char* CallLog::ParseField(const unsigned char *begin,
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

	if( field->type == CLLFC_CALLLOG_TYPE ) {
		if( ( CallLogType = field->u.raw[0] ) != 'p' ) {
			throw Error( "CallLog::ParseField: CallLogType is not 'p'" );
		}
		return begin;
	}

	if( field->type == CLLFC_UNIQUEID)
		return begin;

	// cycle through the type table
	for(    FieldLink<CallLog> *b = CallLogFieldLinks;
		b->type != CLLFC_END;
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
//	switch( field->type )
//	{
//	case CLLFC_STATUS:
//		if( field->u.raw[0] > CLL_STATUS_RANGE_HIGH ) {
//			throw Error( "CallLog::ParseField: status field out of bounds" );
//		}
//		else {
//			StatusFlag = StatusProto2Rec(field->u.raw[0]);
//		}
//		return begin;
//	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void CallLog::ParseHeader(const Data &data, size_t &offset)
{
	// no header in CallLog records
}

void CallLog::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}


void CallLog::Dump(std::ostream &os) const
{
	static const char *StatusName[] = { "Received", "Sent", "Failed" };

	os << "CallLog entry: 0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";

//	os << "   Status: " << StatusName[StatusFlag] << "\n";

	// cycle through the type table
	for(	const FieldLink<CallLog> *b = CallLogFieldLinks;
		b->type != CLLFC_END;
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
				os << "   " << b->name << ": unknown\n";
		}
	}


	os << Unknowns;
	os << "\n\n";
}

void CallLog::Clear()
{
	Phone.clear();
	Name.clear();

	Time = 0;
	CallLogType = 0;
	StatusFlag = (StatusFlagType)0;

	Unknowns.clear();
}

} // namespace Barry

