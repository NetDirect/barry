///
/// \file	r_bookmark.cc
///		Record parsing class for the phone browser bookmarks database.
///

/*
    Copyright (C) 2008-2010, Nicolas VIVIEN
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "r_bookmark.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "iconv.h"
#include <ostream>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// Bookmark Class

// Bookmark Field Codes
#define BMKFC_BOOKMARK_TYPE		0x01

#define BMKFC_NAME			0x04
#define BMKFC_URL			0xff
#define BMKFC_ICON			0x05

#define BMKFC_STRUCT1			0x11
#define BMKFC_STRUCT2			0x12

#define BMKFC_END			0xffff

static FieldLink<Bookmark> BookmarkFieldLinks[] = {
    { BMKFC_NAME,	"Name",  0, 0, &Bookmark::Name, 0, 0, 0, 0, true },
    { BMKFC_URL,	"URL",  0, 0, &Bookmark::URL, 0, 0, 0, 0, true },
    { BMKFC_ICON,	"Icon",  0, 0, &Bookmark::Icon, 0, 0, 0, 0, true },
    { BMKFC_END,	"End of List",   0, 0, 0, 0, 0, 0, 0, false }
};

Bookmark::Bookmark()
{
	Clear();
}

Bookmark::~Bookmark()
{
}

const unsigned char* Bookmark::ParseField(const unsigned char *begin,
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

	if( field->type == BMKFC_BOOKMARK_TYPE ) {
		if( field->u.raw[0] != 'D' ) {
			throw Error( "Bookmark::ParseField: BookmarkType is not 'D'" );
		}
		return begin;
	}


	uint8_t isdefined;

	const unsigned char *b = field->u.raw;
	const unsigned char *e = begin;

	// handle special cases
	switch( field->type )
	{
	case BMKFC_STRUCT1:
		while (b < e) {
			const uint8_t type = *b;

			// advance and check size
			b += 1;
			if( b > e )		// if begin==end, we are ok
				continue;

			switch (type)
			{
			case 0x81:
				b += 8;		// 8 fields unknown
				break;
			case 0x84:
			case 0x85:
				b += 5;		// 4 fields unknown
				break;
			case BMKFC_NAME:
				isdefined = *b;
				b += sizeof(uint8_t);
				if (isdefined == 1) {	// if field is defined
					const uint16_t size = be_btohs(*((const uint16_t *) b));
					b += sizeof(uint16_t);
					Name = ParseFieldString(b, size);
					b += size;
				}
				break;
			case BMKFC_ICON:
				isdefined = *b;
				b += sizeof(uint8_t);
				if (isdefined == 1) {	// if field is defined
					const uint16_t size = be_btohs(*((const uint16_t *) b));
					b += sizeof(uint16_t);
					Icon = ParseFieldString(b, size);
					b += size;
				}
				break;
			case 0x08:
				isdefined = *b;
				b += sizeof(uint8_t);
				if (isdefined == 1) {	// if field is defined
					const uint16_t size = be_btohs(*((const uint16_t *) b));
					b += sizeof(uint16_t);
					b += size;
				}
				break;
			}
		}

		if( btohs(field->size) >= 3 ) {
			if ((Barry::Bookmark::DisplayModeType) *(begin - 3) > Barry::Bookmark::DisplayUnknown)
				DisplayMode = Barry::Bookmark::DisplayUnknown;
			else
				DisplayMode = (Barry::Bookmark::DisplayModeType) *(begin - 3);

			if ((Barry::Bookmark::JavaScriptModeType) *(begin - 2) > Barry::Bookmark::JavaScriptUnknown)
				JavaScriptMode = Barry::Bookmark::JavaScriptUnknown;
			else
				JavaScriptMode = (Barry::Bookmark::JavaScriptModeType) *(begin - 2);

			if ((Barry::Bookmark::BrowserIdentityType) *(begin - 1) > Barry::Bookmark::IdentityUnknown)
				BrowserIdentity = Barry::Bookmark::IdentityUnknown;
			else
				BrowserIdentity = (Barry::Bookmark::BrowserIdentityType) *(begin - 1);
		}

//		return begin;
		break;

	case BMKFC_STRUCT2:
		{
			const uint16_t size = be_btohs(*((const uint16_t *) b));
			b += sizeof(uint16_t);
			URL = ParseFieldString(b, size);
			b += size;
		}

//		return begin;
		break;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void Bookmark::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Bookmark records
}

void Bookmark::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void Bookmark::Dump(std::ostream &os) const
{
	static const char *DisplayModeName[] = { "Automatique", "Enabled", "Disabled", "Unknown" };
	static const char *JavaScriptModeName[] = { "Automatique", "Enabled", "Disabled", "Unknown" };
	static const char *BrowserIdentityName[] = { "Automatique", "BlackBerry", "FireFox", "Internet Explorer", "Unknown" };

	os << "Bookmark entry: 0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")\n";

	// cycle through the type table
	for(	const FieldLink<Bookmark> *b = BookmarkFieldLinks;
		b->type != BMKFC_END;
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

	os << "   Display mode: " << DisplayModeName[DisplayMode] << "\n";
	os << "   JavaScript mode: " << JavaScriptModeName[JavaScriptMode] << "\n";
	os << "   Browser Identity mode: " << BrowserIdentityName[BrowserIdentity] << "\n";

	os << Unknowns;
	os << "\n\n";
}

bool Bookmark::operator<(const Bookmark &other) const
{
	int cmp = Name.compare(other.Name);
	return cmp < 0;
}

void Bookmark::Clear()
{
	RecType = GetDefaultRecType();
	RecordId = 0;

	Name.clear();
	Icon.clear();
	URL.clear();

	BrowserIdentity = IdentityUnknown;
	DisplayMode = DisplayUnknown;
	JavaScriptMode = JavaScriptUnknown;

	Unknowns.clear();
}

} // namespace Barry

