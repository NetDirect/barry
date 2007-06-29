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
#define PNMFC_REPLY_UNKNOWN	0x12	// this appears on replies, always 0x00
#define PNMFC_RECORDID		0x4b	// Internal Message ID, mimics header RecNumber
#define PNMFC_END		0xffff

#define PRIORITY_MASK		0x003f
#define PRIORITY_HIGH		0x0008
#define PRIORITY_LOW		0x0002

#define SENSITIVE_MASK		0xff80
#define SENSITIVE_CONFIDENTIAL	0x0100
#define SENSITIVE_PERSONAL	0x0080
#define SENSITIVE_PRIVATE	0x0040	// actual pattern is 0x00C0

#define MESSAGE_READ		0x0800
#define MESSAGE_REPLY		0x0001
#define MESSAGE_SAVED		0x0002
#define MESSAGE_FORWARD 	0x0008
#define MESSAGE_TRUNCATED	0x0020
#define MESSAGE_SAVED_DELETED	0x0080

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
	// FIXME - we are using a Message (email) record header size
	// for a PIN Message record... this is not necessarily guaranteed
	// to be the same... someday we could use some more info on
	// the message record header and pin message record header
	
	// FIXED - the header structure for both the PIN messages and
	// email messages are the same, as is the header structure for
	// 'Saved Email Messages' although some of the fields may not directly apply.
	//
	// and someday is now here ;) - edge
	
	MAKE_RECORD(const Barry::Protocol::MessageRecord, mr, data, offset);
	// Priority
	MessagePriority = NormalPriority;
	if( mr->priority & PRIORITY_MASK ) {
		if( mr->priority & PRIORITY_HIGH ) {
			MessagePriority = HighPriority;
		}
		else if( mr->priority & PRIORITY_LOW ) {
			MessagePriority = LowPriority;
		}
		else
			MessagePriority = UnknownPriority;
	} 
	// Sensitivity
	MessageSensitivity = NormalSensitivity;
	if( mr->priority & SENSITIVE_MASK ) {
		if(( mr->priority & SENSITIVE_CONFIDENTIAL ) == SENSITIVE_CONFIDENTIAL ) {
			MessageSensitivity = Confidential;
		}
		else if(( mr->priority & SENSITIVE_PRIVATE ) == SENSITIVE_PRIVATE ) {
			MessageSensitivity = Private;
		}
		else if(( mr->priority & SENSITIVE_PERSONAL ) == SENSITIVE_PERSONAL ) {
			MessageSensitivity = Personal;
		}
		else
			MessageSensitivity = UnknownSensitivity;
	}
	// X-rim-org-message-ref-id	// NOTE: I'm cheating a bit here and using this as a reply-to
	if( mr->inReplyTo )		// It's actually sent by BB with the actual UID in every message
		MessageReplyTo = mr->inReplyTo;
	// Status Flags
	if( !( mr->flags & MESSAGE_READ ))
		MessageRead = true;	// NOTE: A lot of these flags are 'backwards' but this seemed
					// like the most logical way to interpret them for now
	if(( mr->flags & MESSAGE_REPLY ) == MESSAGE_REPLY )
		MessageReply = true;	// NOTE: This is a reply, the original message's flags are not changed
					// the inReplyTo field is updated with the original messages's UID
	if( !( mr->flags & MESSAGE_TRUNCATED ))
		MessageTruncated = true;	// NOTE: This bit is unset on truncation, around 4096 on my 7100g
					// NOTE: bit 0x400 is set on REALLY huge messages, haven't tested
					//       the exact size yet
	if( !( mr->flags & MESSAGE_SAVED ))
		MessageSaved = true;	// NOTE: Saved to 'saved' folder
	if( !( mr->flags & MESSAGE_SAVED_DELETED ))
		MessageSavedDeleted = true;	// NOTE: Saved to 'saved' folder and then deleted from inbox
		
	MessageDateSent = ( mr->dateSent & 0x01ff ) - 0x29;
	MessageDateSent = DayToDate( MessageDateSent );
	MessageDateSent += (time_t)( mr->timeSent*1.77 );
	
	MessageDateReceived = ( mr->dateReceived & 0x01ff ) - 0x29;
	MessageDateReceived = DayToDate( MessageDateReceived );
	MessageDateReceived += (time_t)( mr->timeReceived*1.77 );	

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
	MessageReplyTo = 0;
	MessageDateSent = 0;
	MessageDateReceived = 0;
	MessageTruncated = false;
	MessageRead = false;
	MessageReply = false;
	MessageSaved = false;
	MessageSavedDeleted = false;
	
	Unknowns.clear();
}

// dump message in mbox format
void PINMessage::Dump(std::ostream &os) const
{
	static const char *MessageImportance[] = 
		{ "Low", "Normal", "High", "Unknown Priority" };
	static const char *MessageSensitivityString[] = 
		{ "Normal", "Personal", "Private", "Confidential", "Unknown Sensivity" };
	
	os << "From " << (From.Email.size() ? From.Email.c_str() : "unknown")
	   << "  " << ctime( &MessageDateSent );
	os << "X-Record-ID: (" << setw(8) << std::hex << MessageRecordId << ")\n";
	if( MessageReplyTo )
		os << "X-rim-org-msg-ref-id: " << std::dec << MessageReplyTo << "\n";
	if( MessageSaved )
		os << "Message Status: Saved\n";
	else if( MessageRead )
		os << "Message Status: Opened\n";
	if( MessagePriority != NormalPriority )
		os << "Importance: " << MessageImportance[MessagePriority] << "\n";
	if( MessageSensitivity != NormalSensitivity )
		os << "Sensitivity: " << MessageSensitivityString[MessageSensitivity] << "\n";
	os << "Date: " << ctime(&MessageDateSent);
	
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

