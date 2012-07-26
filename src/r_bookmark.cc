///
/// \file	r_bookmark.cc
///		Record parsing class for the phone browser bookmarks database.
///

/*
    Copyright (C) 2008-2010, Nicolas VIVIEN
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
#include "r_bookmark.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "iconv.h"
#include "debug.h"
#include <ostream>
#include <iomanip>
#include <iostream>
#include "ios_state.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// Bookmark Class

// Bookmark Field Codes
#define BMKFC_BOOKMARK_TYPE		0x01
#define BMKFC_STRUCT1			0x11
#define BMKFC_STRUCT2			0x12

#define BMKFC_END			0xffff

// Bookmark Struct1 section codes
#define BMK1SC_HOMEPAGE_KEY		0x85	// default section on 9550...
						// has browser dropdown grayed
						// out
#define BMK1SC_BOOKMARK_ID		0x87	// when user adds a bookmark
#define BMK1SC_BOOKMARK_ID2		0x84	// only Nicolas sees this?
#define BMK1SC_NAME			0x04
#define BMK1SC_ICON			0x05
#define BMK1SC_FOLDERS			0x81

//static FieldLink<Bookmark> BookmarkFieldLinks[] = {
//    { BMKFC_END,	"End of List",   0, 0, 0, 0, 0, 0, 0, false }
//};

Bookmark::Bookmark()
{
	Clear();
}

Bookmark::~Bookmark()
{
}

const unsigned char* Bookmark::ParseStruct1Field(const unsigned char *begin,
					const unsigned char *end,
					const IConverter *ic)
{
	// grab section type
	const uint8_t type = *begin;
	begin += 1;
	if( begin > end )
		return begin;

	switch( type )
	{
	case BMK1SC_HOMEPAGE_KEY:
	case BMK1SC_BOOKMARK_ID:
	case BMK1SC_BOOKMARK_ID2:
		{
			const BookmarkId *id = (const BookmarkId*) begin;
			begin += BOOKMARK_ID_SIZE;
			if( begin > end )
				return begin;

			// not sure where id->bookmark_id links to, so ignore
			// for now...
			Index = id->index;
		}
		return begin;

	case BMK1SC_NAME:
	case BMK1SC_ICON:
		{
			const VarStringField *f = (const VarStringField*) begin;
			begin += VARSTRING_FIELD_HEADER_SIZE;
			if( begin > end )
				return begin;

			const uint16_t size = be_btohs(f->be_size);

			if( f->present == 1) {	// if field is defined
				begin += size;
				if( begin > end )
					return begin;

				switch( type )
				{
				case BMK1SC_NAME:
					Name = ParseFieldString(f->data, size);
					break;
				case BMK1SC_ICON:
					Icon = ParseFieldString(f->data, size);
					break;
				default:
					throw std::logic_error("Bookmark: Check case statement. Should never happen.");
					break;
				}
			}
			else if( f->present == 0 && size > 0xA0 ) {
				// FIXME - a size of 0xA5 seems to occasionally
				// appear, with 5 bytes of id-looking data
				// we just skip it here.... note this
				// may be a different field, but it meshes
				// itself into the ICON section somehow,
				// that is unclear
//
// example with the A5, before modification to add '?'
//    Type: 0x11 Data:
// 00000000: 85 9b ed ca 13 00 04 01 00 09 48 6f 6d 65 20 50  ..........Home P
// 00000010: 61 67 65 81 b9 fc f8 f6 c2 e3 a4 d5 08 01 05 00  age.............
// 00000020: 00 a5 b8 f0 97 e4 3a 00 00 01                    ......:...
//
// example without the A5, after record modified:
//    Type: 0x11 Data:
// 00000000: 85 9b ed ca 13 00 04 01 00 0a 48 6f 6d 65 20 50  ..........Home P
// 00000010: 61 67 65 3f 81 b9 fc f8 f6 c2 e3 a4 d5 08 01 05  age?............
// 00000020: 00 00 00 00 00 01                                ......
//

				begin += size & 0x0F;
			}
		}
		return begin;

	case BMK1SC_FOLDERS:
		{
			//const BookmarkFolders *f = (const BookmarkFolders*) begin;
			begin += BOOKMARK_FOLDERS_HEADER_SIZE;
			if( begin > end )
				return begin;

			// currently don't know how to link these to
			// anything else in the device.... skipping
		}
		return begin;

/*
	case 0x08:
		isdefined = *b;
		b += sizeof(uint8_t);
		if (isdefined == 1) {	// if field is defined
			const uint16_t size = be_btohs(*((const uint16_t *) b));
			b += sizeof(uint16_t);
			b += size;
		}
		break;
*/

	default:
		// if we are 3 bytes away from the end, these are the
		// display, javascript, and browser identity flags
		// (make sure to account for the type we ate above)
		if( (end - begin) == 2 ) {
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
		// if we are at the beginning, this could be the 7750
		// with its odd no-code ID... the 7750 seems to have
		// a BOOKMARK_ID record without any code byte,
		// so check that the data looks like this:
		//    XX XX XX XX 00
		// with the 4 byte ID and the index of 0, being the
		// first default bookmark
		else if( (begin + 3) < end && begin[3] == 0 ) {
			// recurse into ourselves
			return ParseStruct1Field(begin + 4, end, ic);
		}
		else {
			ddout("Bookmark parser: unknown section type: "
				<< std::hex << (unsigned int) type);
		}
		break;
	}

	return begin;
}

const unsigned char* Bookmark::ParseStruct2(const unsigned char *begin,
					const unsigned char *end,
					const IConverter *ic)
{
	// first field in struct2 seems to always be the URL

	// grab size and advance over string, checking sizes
	const StringField *field = (const StringField*) begin;
	begin += STRING_FIELD_HEADER_SIZE;
	if( begin > end )
		return begin;

	const uint16_t size = be_btohs(field->be_size);
	begin += sizeof(uint16_t) + size;
	if( begin > end )	// if begin==end, we are ok
		return begin;

	Url = ParseFieldString(field->data, size);

	// FIXME - more fields after this, but unknown meaning

	return begin;
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


	const unsigned char *b = field->u.raw;
	const unsigned char *e = begin;

	// handle special cases
	switch( field->type )
	{
	case BMKFC_STRUCT1:
		while (b <= e) {
			b = ParseStruct1Field(b, e, ic);
		}
		return b;

	case BMKFC_BOOKMARK_TYPE:
		// above size check guarantees at least one byte,
		// so this is safe
		if( field->u.raw[0] != 'D' ) {
			throw Error( _("Bookmark::ParseField: BookmarkType is not 'D'") );
		}
		return begin;

	case BMKFC_STRUCT2:
		begin = ParseStruct2(b, e, ic);
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

void Bookmark::Validate() const
{
}

void Bookmark::BuildHeader(Data &data, size_t &offset) const
{
	// not yet implemented
}

void Bookmark::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	// not yet implemented
}

void Bookmark::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	static const char *DisplayModeName[] = {
		N_("Automatic"),
		N_("Enabled"),
		N_("Disabled"),
		N_("Unknown")
	};
	static const char *JavaScriptModeName[] = {
		N_("Automatic"),
		N_("Enabled"),
		N_("Disabled"),
		N_("Unknown")
	};
	static const char *BrowserIdentityName[] = {
		N_("Automatic"),
		N_("BlackBerry"),
		N_("FireFox"),
		N_("Internet Explorer"),
		N_("Unknown")
	};

	os << _("Bookmark entry: ") << "0x" << setbase(16) << RecordId
	   << " (" << (unsigned int)RecType << ")"
	   << " (" << _("index ") << (unsigned int)Index << ")\n";

	if( Name.size() )
		os << _("                    Name: ") << Name << "\n";
	if( Icon.size() )
		os << _("                    Icon: ") << Icon << "\n";
	if( Url.size() )
		os << _("                     Url: ") << Url << "\n";
	os << _("            Display mode: ")
		<< gettext( DisplayModeName[DisplayMode] ) << "\n";
	os << _("         JavaScript mode: ")
		<< gettext( JavaScriptModeName[JavaScriptMode] ) << "\n";
	os << _("   Browser Identity mode: ")
		<< gettext( BrowserIdentityName[BrowserIdentity] ) << "\n";

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
	Index = 0;

	Name.clear();
	Icon.clear();
	Url.clear();

	BrowserIdentity = IdentityUnknown;
	DisplayMode = DisplayUnknown;
	JavaScriptMode = JavaScriptUnknown;

	Unknowns.clear();
}

const FieldHandle<Bookmark>::ListT& Bookmark::GetFieldHandles()
{
	static FieldHandle<Bookmark>::ListT fhv;

	if( fhv.size() )
		return fhv;

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME fhv

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME Bookmark

	FHP(RecType, _("Record Type"));
	FHP(RecordId, _("Unique Record ID"));

	FHP(Index, _("Bookmark Field Index"));

	FHP(Name, _("Site Name"));
	FHP(Icon, _("Site Icon"));
	FHP(Url, _("Site URL"));

	FHE(bit, BrowserIdentityType, BrowserIdentity, _("Browser Identity"));
	FHE_CONST(bit, IdentityAuto, _("Auto detect browser"));
	FHE_CONST(bit, IdentityBlackBerry, _("BlackBerry browser"));
	FHE_CONST(bit, IdentityFireFox, _("FireFox browser"));
	FHE_CONST(bit, IdentityInternetExplorer, _("Internet Explorer browser"));
	FHE_CONST(bit, IdentityUnknown, _("Unknown browser"));

	FHE(dmc, DisplayModeType, DisplayMode, _("Display Mode"));
	FHE_CONST(dmc, DisplayAuto, _("Automatic"));
	FHE_CONST(dmc, DisplayColomn, _("Column"));
	FHE_CONST(dmc, DisplayPage, _("Page"));
	FHE_CONST(dmc, DisplayUnknown, _("Unknown"));

	FHE(jsm, JavaScriptModeType, JavaScriptMode, _("JavaScript Mode"));
	FHE_CONST(jsm, JavaScriptAuto, _("Automatic"));
	FHE_CONST(jsm, JavaScriptEnabled, _("Enabled"));
	FHE_CONST(jsm, JavaScriptDisabled, _("Disabled"));
	FHE_CONST(jsm, JavaScriptUnknown, _("Unknown"));

	FHP(Unknowns, _("Unknown Fields"));

	return fhv;
}

std::string Bookmark::GetDescription() const
{
	return Name;
}

} // namespace Barry

