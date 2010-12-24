///
/// \file	r_memo.cc
///		Record parsing class for the memo database.
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)
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
#include "iconv.h"
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

static FieldLink<Memo> MemoFieldLinks[] = {
    { MEMFC_TITLE,     "Title",       0, 0, &Memo::Title, 0, 0, 0, 0, true },
    { MEMFC_BODY,      "Body",        0, 0, &Memo::Body, 0, 0, 0, 0, true },
    { MEMFC_END,       "End of List", 0, 0, 0, 0, 0, 0, 0, false }
};

Memo::Memo()
{
	Clear();
}

Memo::~Memo()
{
}

const unsigned char* Memo::ParseField(const unsigned char *begin,
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

	if( field->type == MEMFC_MEMO_TYPE ) {
		if( field->u.raw[0] != 'm' ) {
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
	case MEMFC_CATEGORY:
		{
			std::string catstring = ParseFieldString(field);
			if( ic )
				catstring = ic->FromBB(catstring);
			Categories.CategoryStr2List(catstring);
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

void Memo::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Memo records
}

void Memo::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}


void Memo::BuildHeader(Data &data, size_t &offset) const
{
	// no header in Memo records
}


//
// Build
//
/// Build fields part of record.
///
void Memo::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	data.Zap();

	// tack on the 'm' memo type field first
	BuildField(data, offset, MEMFC_MEMO_TYPE, 'm');

	// Categories
	if( Categories.size() ) {
		string store;
		Categories.CategoryList2Str(store);
		BuildField(data, offset, MEMFC_CATEGORY, ic ? ic->ToBB(store) : store);
	}

	// cycle through the type table
	for(	FieldLink<Memo> *b = MemoFieldLinks;
		b->type != MEMFC_END;
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
		else if( b->postMember && b->postField ) {
			const std::string &field = (this->*(b->postMember)).*(b->postField);
			if( field.size() ) {
				std::string s = (b->iconvNeeded && ic) ? ic->ToBB(field) : field;
				BuildField(data, offset, b->type, s);
			}
		}
	}

	// and finally save unknowns
	UnknownsType::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	for( ; ub != ue; ub++ ) {
		BuildField(data, offset, *ub);
	}

	data.ReleaseBuffer(offset);
}



void Memo::Dump(std::ostream &os) const
{
	os << "Memo entry: 0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";
	os << "    Title: " << Title << "\n";
	os << "    Body: ";

	// The Body may have '\r' characters... translate them
	// in the output to make it look more pretty
	for( string::const_iterator i = Body.begin(); i != Body.end(); ++i ) {
		if( *i == '\r' )
			os << "\n          ";
		else
			os << *i;
	}
	os << "\n";

	if( Categories.size() ) {
		string display;
		Categories.CategoryList2Str(display);
		os << "    Categories: " << display << "\n";
	}

	os << Unknowns;
	os << "\n\n";
}

bool Memo::operator<(const Memo &other) const
{
	int cmp = Title.compare(other.Title);
	if( cmp == 0 )
		cmp = Body.compare(other.Body);
	return cmp < 0;
}

void Memo::Clear()
{
	RecType = GetDefaultRecType();
	RecordId = 0;

	Title.clear();
	Body.clear();
	Categories.clear();

	Unknowns.clear();
}


} // namespace Barry

