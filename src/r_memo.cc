///
/// \file	r_memo.cc
///		Record parsing class for the memo database.
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)
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

#include "r_memo.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include <ostream>
#include <iomanip>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// Memo Class

// Memo Field Codes
#define MEMFC_TITLE		0x01
#define MEMFC_BODY		0x02
#define MEMFC_MEMO_TYPE		0x03
#define MEMFC_CATEGORY		0x04
#define MEMFC_END		0xffff

FieldLink<Memo> MemoFieldLinks[] = {
	{ MEMFC_TITLE,     "Title",       0, 0, &Memo::Title, 0, 0 },
	{ MEMFC_BODY,      "Body",        0, 0, &Memo::Body, 0, 0 },
	{ MEMFC_CATEGORY,  "Category",    0, 0, &Memo::Category, 0, 0 },
	{ MEMFC_END,       "End of List", 0, 0, 0, 0, 0 }
};

Memo::Memo()
{
	Clear();
}

Memo::~Memo()
{
}

const unsigned char* Memo::ParseField(const unsigned char *begin,
				      const unsigned char *end)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if( begin > end )       // if begin==end, we are ok
		return begin;

	if( !btohs(field->size) )   // if field has no size, something's up
		return begin;

	if( field->type == MEMFC_MEMO_TYPE ) {
		if( ( MemoType = field->u.raw[0] ) != 'm' ) {
			throw Error( "Memo::ParseField: MemoType is not 'm'" );
		}
	return begin;
	}


	// cycle through the type table
	for(    FieldLink<Memo> *b = MemoFieldLinks;
		b->type != MEMFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s.assign((const char *)field->u.raw, btohs(field->size)-1);
				return begin;   // done!
			}
			else if( b->timeMember && btohs(field->size) == 4 ) {
				time_t &t = this->*(b->timeMember);
				t = min2time(field->u.min1900);
				return begin;
			}
		}
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void Memo::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Memo records
}

void Memo::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
	data.GetData() + offset, data.GetData() + data.GetSize());
	offset += finish - (data.GetData() + offset);
}


void Memo::Dump(std::ostream &os) const
{
	os << "Memo entry: 0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";
	os << "    Title: " << Title << "\n";
	os << "    Body: " << Body << "\n";
	os << "    Category: " << Category << "\n";

	os << Unknowns;
	os << "\n\n";
}

void Memo::Clear()
{
	Title.clear();
	Body.clear();
	Category.clear();

	MemoType = 0;

	Unknowns.clear();
}

} // namespace Barry

