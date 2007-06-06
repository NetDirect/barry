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
#define PNMFC_SUBJECT	0x0b
#define PNMFC_BODY		0x0c
#define PNMFC_RECORDID	0x4b	// Internal Message ID, mimics header RecNumber
#define PNMFC_END		0xffff

FieldLink<PINMessage> PINMessageFieldLinks[] = {
		{ PNMFC_TO,		"To",		0, 0,    0, &PINMessage::To,  0 },
		{ PNMFC_CC,		"Cc",		0, 0,    0, &PINMessage::Cc, 0 },
		{ PNMFC_BCC,	"Bcc",		0, 0,    0, &PINMessage::Bcc, 0 },
		{ PNMFC_FROM,	"From",		0, 0,    0, &PINMessage::From, 0 },
		{ PNMFC_SUBJECT	"Subject",	0, 0,    &PINMessage::Subject, 0, 0 },
		{ PNMFC_BODY,	"Body",		0, 0,    &PINMessage::Body, 0, 0 },
		{ PNMFC_END,	"End of List",	0, 0,    0, 0, 0 }
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
		MessageRecordId = field->u.min1900; // not really time, but we need easy access to an int
		return begin;
	}
	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);
	
	return begin;
}

uint8_t PINMessage::GetRecType() const
{
	throw std::logic_error("PINMessage::GetRecType() called, and not supported by the USB protocol.  Should never get called.");
}

// empty API, not required by protocol
uint32_t PINMessage::GetUniqueId() const
{
	throw std::logic_error("PINMessage::GetUniqueId() called, and not supported by the USB protocol.  Should never get called.");
}

// empty API, not required by protocol
void PINMessage::SetIds(uint8_t Type, uint32_t Id)
{
	// accept it without complaining, just do nothing
}

void PINMessage::ParseHeader(const Data &data, size_t &offset)
{
	// we skip the "header" since we don't know what to do with it yet
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

	From.Name.clear();
	From.Email.clear();

	To.Name.clear();
	To.Email.clear();

	Cc.Name.clear();
	Cc.Email.clear();
	
	Bcc.Name.clear();
	Bcc.Email.clear();

	Subject.clear();
	Body.clear();
	MessageRecordId = 0;
	
	Unknowns.clear();
}

// dump message in mbox format
void PINMessage::Dump(std::ostream &os) const
{
	os << "Record ID  (" << MessageRecordId << ")\n";
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

