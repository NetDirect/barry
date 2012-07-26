///
/// \file	r_hhagent.cc
///		Blackberry database record parser class for Handheld Agent records
///

/*
    Copyright (C) 2011-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "r_hhagent.h"
#include "record-internal.h"
#include "protostructs.h"
#include "iconv.h"
#include "time.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include "ios_state.h"

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {


///////////////////////////////////////////////////////////////////////////////
// HandheldAgent class

// Common field codes
#define HHAFC_END		0xffff

// Field codes for record 3000000
#define HHAFC3_MODEL		0x03
#define HHAFC3_BANDS		0x07
#define HHAFC3_VERSION		0x08
#define HHAFC3_NETWORK		0x0f
#define HHAFC3_PIN		0x10
#define HHAFC3_MEID		0x11

// Field codes for record 7000000
#define HHAFC7_FIRMWARE		0x13
#define HHAFC7_MANUFACTURER	0x14
#define HHAFC7_MODEL		0x15
#define HHAFC7_PLATFORM		0x17

// These fields are only valid for RecordId 0x3000000
static FieldLink<HandheldAgent> HandheldAgentFieldLinks_3000000[] = {
   { HHAFC3_MODEL,     N_("Model"),      0, 0, &HandheldAgent::Model, 0, 0, 0, 0, true },
   { HHAFC3_NETWORK,   N_("Network"),    0, 0, &HandheldAgent::Network, 0, 0, 0, 0, true },
   { HHAFC3_BANDS,     N_("Bands"),      0, 0, &HandheldAgent::Bands, 0, 0, 0, 0, true },
   { HHAFC3_MEID,      N_("MEID/ESN"),   0, 0, &HandheldAgent::MEID, 0, 0, 0, 0, true },
   { HHAFC3_PIN,       N_("PIN"),        0, 0, &HandheldAgent::Pin, 0, 0, 0, 0, true },
   { HHAFC3_VERSION,   N_("Version"),0, 0, &HandheldAgent::Version, 0, 0, 0, 0, true },
   { HHAFC_END,        N_("End of List"),0, 0, 0, 0, 0, 0, 0, false }
};

// These fields are only for RecordId 0x4000000
static FieldLink<HandheldAgent> HandheldAgentFieldLinks_4000000[] = {
   { HHAFC_END,        N_("End of List"),0, 0, 0, 0, 0, 0, 0, false }
};

// These fields are only for RecordId 0x5000000
static FieldLink<HandheldAgent> HandheldAgentFieldLinks_5000000[] = {
   { HHAFC_END,        N_("End of List"),0, 0, 0, 0, 0, 0, 0, false }
};

// These fields are only for RecordId 0x7000000
static FieldLink<HandheldAgent> HandheldAgentFieldLinks_7000000[] = {
   { HHAFC7_MODEL,     N_("Model"),    0, 0, &HandheldAgent::Model, 0, 0, 0, 0, true },
   { HHAFC7_MANUFACTURER,N_("Manufacturer"),0,0,&HandheldAgent::Manufacturer,0, 0, 0, 0, true },
   { HHAFC7_FIRMWARE,  N_("Firmware"), 0, 0, &HandheldAgent::Version, 0, 0, 0, 0, true },
   { HHAFC7_PLATFORM,  N_("Platform"), 0, 0, &HandheldAgent::PlatformVersion, 0, 0, 0, 0, true },
   { HHAFC_END,        N_("End of List"),0, 0, 0, 0, 0, 0, 0, false }
};

// Use this table for default application style records
static FieldLink<HandheldAgent> HandheldAgentFieldLinks_Default[] = {
   { HHAFC_END,        N_("End of List"),0, 0, 0, 0, 0, 0, 0, false }
};

// Use this for display / Dump() etc... includes all fields
static FieldLink<HandheldAgent> HandheldAgentFieldLinks_All[] = {
   { 0, N_("Model"),      0, 0, &HandheldAgent::Model, 0, 0, 0, 0, true },
   { 0, N_("Network"),    0, 0, &HandheldAgent::Network, 0, 0, 0, 0, true },
   { 0, N_("Manufacturer"),0,0, &HandheldAgent::Manufacturer,0, 0, 0, 0, true },
   { 0, N_("Bands"),      0, 0, &HandheldAgent::Bands, 0, 0, 0, 0, true },
   { 0, N_("MEID/ESN"),   0, 0, &HandheldAgent::MEID, 0, 0, 0, 0, true },
   { 0, N_("PIN"),        0, 0, &HandheldAgent::Pin, 0, 0, 0, 0, true },
   { 0, N_("Version"),    0, 0, &HandheldAgent::Version, 0, 0, 0, 0, true },
   { 0, N_("Platform"),   0, 0, &HandheldAgent::PlatformVersion, 0, 0, 0, 0, true },
   { HHAFC_END,        N_("End of List"),0, 0, 0, 0, 0, 0, 0, false }
};

HandheldAgent::HandheldAgent()
{
	Clear();
}

HandheldAgent::~HandheldAgent()
{
}

const unsigned char* HandheldAgent::ParseField(const unsigned char *begin,
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
	FieldLink<HandheldAgent> *b = HandheldAgentFieldLinks_Default;
	if( RecordId == 0 ) {
		// internal consistency check... all parsing code should
		// call SetIds() first, and HandheldAgent relies on this,
		// so double check, and throw if not
		throw std::logic_error(_("HandheldAgent requires SetIds() to be called before ParseField()"));
	}
	else if( RecordId == GetMEIDRecordId() ) {
		b = HandheldAgentFieldLinks_3000000;
	}
	else if( RecordId == GetUnknown1RecordId() ) {
		b = HandheldAgentFieldLinks_4000000;
	}
	else if( RecordId == GetUnknown2RecordId() ) {
		b = HandheldAgentFieldLinks_5000000;
	}
	else if( RecordId == GetUnknown3RecordId() ) {
		b = HandheldAgentFieldLinks_7000000;
	}

	for( ; b->type != HHAFC_END; b++ ) {
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
//	switch( field->type )
//	{
//	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void HandheldAgent::ParseHeader(const Data &data, size_t &offset)
{
	// no header in HandheldAgent records
}

void HandheldAgent::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void HandheldAgent::Validate() const
{
}

void HandheldAgent::BuildHeader(Data &data, size_t &offset) const
{
	// no header in HandheldAgent records
}

//
// Build
//
/// Build fields part of record.
///
void HandheldAgent::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
}

void HandheldAgent::Clear()
{
	// clear our fields
	RecType = GetDefaultRecType();
	RecordId = 0;

	MEID.clear();
	Model.clear();
	Bands.clear();
	Pin.clear();
	Version.clear();
	PlatformVersion.clear();
	Manufacturer.clear();
	Network.clear();

	Unknowns.clear();
}

const FieldHandle<HandheldAgent>::ListT& HandheldAgent::GetFieldHandles()
{
	static FieldHandle<HandheldAgent>::ListT fhv;

	if( fhv.size() )
		return fhv;

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME fhv

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME HandheldAgent

	FHP(RecType, _("Record Type Code"));
	FHP(RecordId, _("Unique Record ID"));

	// These fields are only valid for RecordId 0x3000000
	FHD(MEID, _("MEID/ESN"), HHAFC3_MEID, true);
	FHD(Model, _("Model"), HHAFC3_MODEL, true);
	FHD(Bands, _("Bands"), HHAFC3_BANDS, true);
	FHD(Pin, _("PIN"), HHAFC3_PIN, true);
	FHD(Version, _("Version"), HHAFC3_VERSION, true);
	FHD(Network, _("Network"), HHAFC3_NETWORK, true);

	// These fields are only for RecordId 0x7000000
	FHD(PlatformVersion, _("Platform Version"), HHAFC7_PLATFORM, true);
	FHD(Manufacturer, _("Manufacturer"), HHAFC7_MANUFACTURER, true);

	FHP(Unknowns, _("Unknown Fields"));

	return fhv;
}

std::string HandheldAgent::GetDescription() const
{
	ostringstream oss;
	oss << _("Handheld Agent: ") << "0x" << hex << RecordId;
	return oss.str();
}

void HandheldAgent::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	os << _("HandheldAgent entry: ") << "0x" << hex << RecordId
		<< " (" << (unsigned int)RecType << ")\n";

	// cycle through the type table
	for(	const FieldLink<HandheldAgent> *b = HandheldAgentFieldLinks_All;
		b->type != HHAFC_END;
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
				os << "   " << gettext(b->name) << ": disabled\n";
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

	// print any unknowns
	os << Unknowns;
}

bool HandheldAgent::operator<(const HandheldAgent &other) const
{
	return RecordId < other.RecordId;
}

bool HandheldAgent::IsSpecial(uint32_t record_id)
{
	return
		record_id == GetMEIDRecordId() ||
		record_id == GetUnknown1RecordId() ||
		record_id == GetUnknown2RecordId() ||
		record_id == GetUnknown3RecordId();
}

//
// The ESN number is in two parts.  When in decimal, the first 3
// characters are one number, then the rest.  In hex, the first 2
// digits are the same number, then the rest.  Both are padded
// with zeros.
//
// For example, hex: 4c070068   dec: 07600458856
//        hex: [4c]070068   dec: [076]00458856
//
// Returns an empty string on error.
//

bool HandheldAgent::IsESNHex(const std::string &esn)
{
	const char *hex = "0123456789ABCDEFabcdef";
	size_t npos = string::npos;

	if( esn.size() == 8 && esn.find_first_not_of(hex) == npos ) {
		return true;
	}

	return false;
}

bool HandheldAgent::IsESNDec(const std::string &esn)
{
	const char *dec = "0123456789";
	size_t npos = string::npos;

	if( esn.size() == 11 && esn.find_first_not_of(dec) == npos ) {
		return true;
	}

	return false;
}

std::string HandheldAgent::ESNDec2Hex(const std::string &esn)
{
	string empty;

	if( esn.size() != 11 )
		return empty;

	unsigned int part1, part2;
	istringstream iss(esn.substr(0, 3));
	iss >> dec >> part1;
	if( !iss )
		return empty;
	iss.str(esn.substr(3));
	iss.clear();
	iss >> dec >> part2;
	if( !iss )
		return empty;

	ostringstream oss;
	oss << setfill('0') << setw(2) << hex << part1;
	oss << setfill('0') << setw(6) << hex << part2;
	if( !oss )
		return empty;
	return oss.str();
}

std::string HandheldAgent::ESNHex2Dec(const std::string &esn)
{
	string empty;

	if( esn.size() != 8 )
		return empty;

	unsigned int part1, part2;
	istringstream iss(esn.substr(0, 2));
	iss >> hex >> part1;
	if( !iss )
		return empty;
	iss.str(esn.substr(2));
	iss.clear();
	iss >> hex >> part2;
	if( !iss )
		return empty;

	ostringstream oss;
	oss << setfill('0') << setw(3) << dec << part1;
	oss << setfill('0') << setw(8) << dec << part2;
	if( !oss )
		return empty;
	return oss.str();
}

} // namespace Barry

