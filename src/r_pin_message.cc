///
/// \file	r_pin_message.cc
///		Blackberry database record parser class for pin message records.
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2007, Brian Edginton (edge@edginton.net)

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

#include "r_pin_message.h"
#include "record-internal.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "error.h"
#include "endian.h"
#include <ostream>
#include <iomanip>
#include <time.h>
#include <stdexcept>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

//std::ostream& operator<<(std::ostream &os, const Address &msgp) {
//	os << msgp.Name.c_str() << " <" << msgp.Email.c_str() << ">";
//	return os;
//}

///////////////////////////////////////////////////////////////////////////////
// PINMessage class


// PIN message field codes
#define PNMFC_TO		0x01		// can occur multiple times
#define PNMFC_CC		0x02		// ditto
#define PNMFC_BCC		0x03		// ditto
#define PNMFC_FROM		0x05
#define PNMFC_SUBJECT		0x0b
#define PNMFC_BODY		0x0c
#define PNMFC_RECORDID		0x4b	// Internal Message ID, mimics header RecNumber
#define PNMFC_END		0xffff

FieldLink<PINMessage> PINMessageFieldLinks[] = {
   { PNMFC_TO,      "To",          0, 0,    0, &PINMessage::To,  0 },
   { PNMFC_CC,      "Cc",          0, 0,    0, &PINMessage::Cc, 0 },
   { PNMFC_BCC,     "Bcc",         0, 0,    0, &PINMessage::Bcc, 0 },
   { PNMFC_FROM,    "From",        0, 0,    0, &PINMessage::From, 0 },
   { PNMFC_SUBJECT, "Subject",     0, 0,    &PINMessage::Subject, 0, 0 },
   { PNMFC_BODY,    "Body",        0, 0,    &PINMessage::Body, 0, 0 },
   { PNMFC_END,     "End of List", 0, 0,    0, 0, 0 }
};

PINMessage::PINMessage()
{
}

PINMessage::~PINMessage()
{
}

const unsigned char* PINMessage::ParseField(const unsigned char *begin,
					 const unsigned char *end)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !btohs(field->size) )	// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	FieldLink<PINMessage> *b = PINMessageFieldLinks;
		b->type != PNMFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				// parse regular string
				std::string &s = this->*(b->strMember);
				s.assign((const char *)field->u.raw, btohs(field->size)-1);
				return begin;	// done!
			}
			else if( b->addrMember ) {
				// parse email address
				// get dual name+addr string first
				const char *fa = (const char*)field->u.addr.addr;
				std::string dual(fa, btohs(field->size) - sizeof(field->u.addr.unknown));

				// assign first string, using null terminator...letting std::string add it for us if it doesn't exist
				Address &a = this->*(b->addrMember);
				a.Name = dual.c_str();

				// assign second string, using first size as starting point
				a.Email = dual.c_str() + a.Name.size() + 1;
				return begin;
			}
		}
	}

	// handle special cases
	switch( field->type )
	{
	case PNMFC_RECORDID:
		MessageRecordId = field->u.uint32;
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	return begin;
}

void PINMessage::ParseHeader(const Data &data, size_t &offset)
{
	// we skip the "header" since we don't know what to do with it yet
	// FIXME - we are using a Message (email) record header size
	// for a PIN Message record... this is not necessarily guaranteed
	// to be the same... someday we could use some more info on
	// the message record header and pin message record header
	offset += MESSAGE_RECORD_HEADER_SIZE;
}

void PINMessage::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize());
	offset += finish - (data.GetData() + offset);
}

void PINMessage::BuildHeader(Data &data, size_t &offset) const
{
	throw std::logic_error("PINMessage::BuildHeader not yet implemented");
}

void PINMessage::BuildFields(Data &data, size_t &offset) const
{
	throw std::logic_error("PINMessage::BuildFields not yet implemented");
}

void PINMessage::Clear()
{
	From.clear();
	To.clear();
	Cc.clear();
	Bcc.clear();

	Subject.clear();
	Body.clear();
	MessageRecordId = 0;

	Unknowns.clear();
}

// dump message in mbox format
void PINMessage::Dump(std::ostream &os) const
{
	// FIXME - use current time until we figure out the date headers
	time_t fixme = time(NULL);
	
	os << "From " << (From.Email.size() ? From.Email.c_str() : "unknown")
	   << "  " << ctime(&fixme);
	os << "X-Record-ID: (" << std::hex << MessageRecordId << ")\n";
	if( From.Name.size()) {
		os << "    From: " << From.Name << " <" << From.Email << ">\n";
	}
	if( To.Name.size()) {
		os << "    To: " << To.Name << " <" << To.Email << ">\n";
	}
	if( Cc.Name.size()) {
		os << "    Cc: " << Cc.Name << " <" << Cc.Email << ">\n";
	}
	if( Bcc.Name.size()) {
		os << "    Bcc: " << Bcc.Name << " <" << Bcc.Email << ">\n";
	}

	if( Subject.size() )
		os << "    Subject: " << Subject << "\n";
	else 
		os << "    Subject: <>\n";
	os << "\n";

	for(	std::string::const_iterator i = Body.begin();
		i != Body.end() && *i;
		i++)
	{
		if( *i == '\r' )
			os << '\n';
		else
			os << *i;
	}
	os << "\n";

	os << Unknowns;
	os << "\n\n";
}


} // namespace Barry

