///
/// \file	r_message.cc
///		Blackberry database record parser class for email records.
///

/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "r_message.h"
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

///////////////////////////////////////////////////////////////////////////////
// Message class


// Email / message field codes
#define MFC_TO			0x01		// can occur multiple times
#define MFC_CC			0x02		// ditto
#define MFC_BCC			0x03		// ditto
#define MFC_SENDER		0x04
#define MFC_FROM		0x05
#define MFC_REPLY_TO		0x06
#define MFC_SUBJECT		0x0b
#define MFC_BODY		0x0c
#define MFC_REPLY_UNKNOWN	0x12	// This shows up as 0x00 on replies but we don't do much with it now
#define MFC_ATTACHMENT		0x16
#define MFC_RECORDID		0x4b
#define MFC_END		0xffff

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

static FieldLink<Message> MessageFieldLinks[] = {
   { MFC_TO,            "To",           0, 0,    0, &Message::To, 0 },
   { MFC_CC,            "Cc",           0, 0,    0, &Message::Cc, 0 },
   { MFC_BCC,           "Bcc",          0, 0,    0, &Message::Bcc, 0 },
   { MFC_SENDER,        "Sender",       0, 0,    0, &Message::Sender, 0 },
   { MFC_FROM,          "From",         0, 0,    0, &Message::From, 0 },
   { MFC_REPLY_TO,      "ReplyTo",      0, 0,    0, &Message::ReplyTo, 0 },
   { MFC_SUBJECT,       "Subject",      0, 0,    &Message::Subject, 0, 0 },
   { MFC_BODY,          "Body",         0, 0,    &Message::Body, 0, 0 },
   { MFC_ATTACHMENT,    "Attachment",   0, 0,    &Message::Attachment, 0, 0 },
   { MFC_END,           "End of List",  0, 0,    0, 0, 0 }
};

Message::Message()
{
	Clear();
}

Message::~Message()
{
}

const unsigned char* Message::ParseField(const unsigned char *begin,
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
	for(	FieldLink<Message> *b = MessageFieldLinks;
		b->type != MFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				// parse regular string
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				return begin;	// done!
			}
			else if( b->addrMember ) {
				// parse email address
				// get dual name+addr string first
				const char *fa = (const char*)field->u.addr.addr;
				std::string dual(fa, btohs(field->size) - sizeof(field->u.addr.unknown));

				// assign first string, using null terminator...letting std::string add it for us if it doesn't exist
				EmailAddress &a = this->*(b->addrMember);
				a.Name = dual.c_str();

				// assign second string, using first size as starting point
				a.Email = dual.c_str() + a.Name.size() + 1;
				return begin;
			}
		}
	}
	// handle special cases
	char swallow;
	switch( field->type )
	{
	case MFC_RECORDID:
		MessageRecordId = btohl(field->u.uint32);
		return begin;
	case MFC_REPLY_UNKNOWN:
		swallow = field->u.raw[0];
		return begin;
	}
	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);
	
	return begin;
}

uint8_t Message::GetRecType() const
{
	throw std::logic_error("Message::GetRecType() called, and not supported by the USB protocol.  Should never get called.");
}

// empty API, not required by protocol
uint32_t Message::GetUniqueId() const
{
	throw std::logic_error("Message::GetUniqueId() called, and not supported by the USB protocol.  Should never get called.");
}

void Message::ParseHeader(const Data &data, size_t &offset)
{
	Protocol::CheckSize(data, offset + MESSAGE_RECORD_HEADER_SIZE);

	MAKE_RECORD(const Barry::Protocol::MessageRecord, mr, data, offset);

	// Priority
	MessagePriority = NormalPriority;

	uint16_t priority = btohs(mr->priority);  // deal with endian swap once
	if( priority & PRIORITY_MASK ) {
		if( priority & PRIORITY_HIGH ) {
			MessagePriority = HighPriority;
		}
		else if( priority & PRIORITY_LOW ) {
			MessagePriority = LowPriority;
		}
		else
			MessagePriority = UnknownPriority;
	} 
	// Sensitivity
	MessageSensitivity = NormalSensitivity;
	if( priority & SENSITIVE_MASK ) {
		if(( priority & SENSITIVE_CONFIDENTIAL ) == SENSITIVE_CONFIDENTIAL ) {
			MessageSensitivity = Confidential;
		}
		else if(( priority & SENSITIVE_PRIVATE ) == SENSITIVE_PRIVATE ) {
			MessageSensitivity = Private;
		}
		else if(( priority & SENSITIVE_PERSONAL ) == SENSITIVE_PERSONAL ) {
			MessageSensitivity = Personal;
		}
		else
			MessageSensitivity = UnknownSensitivity;
	}
	// X-rim-org-message-ref-id	// NOTE: I'm cheating a bit here and using this as a reply-to
	if( mr->inReplyTo )		// It's actually sent by BB with the actual UID in every message
		MessageReplyTo = btohl(mr->inReplyTo);

	// Status Flags
	uint32_t flags = btohl(mr->flags);
	if( !( flags & MESSAGE_READ ))
		MessageRead = true;	// NOTE: A lot of these flags are 'backwards' but this seemed
					// like the most logical way to interpret them for now
	if(( flags & MESSAGE_REPLY ) == MESSAGE_REPLY )
		MessageReply = true;	// NOTE: This is a reply, the original message's flags are not changed
					// the inReplyTo field is updated with the original messages's UID
	if( !( flags & MESSAGE_TRUNCATED ))
		MessageTruncated = true;	// NOTE: This bit is unset on truncation, around 4096 on my 7100g
					// NOTE: bit 0x400 is set on REALLY huge messages, haven't tested
					//       the exact size yet
	if( !( flags & MESSAGE_SAVED ))
		MessageSaved = true;	// NOTE: Saved to 'saved' folder
	if( !( flags & MESSAGE_SAVED_DELETED ))
		MessageSavedDeleted = true;	// NOTE: Saved to 'saved' folder and then deleted from inbox

	MessageDateSent = Message2Time(mr->dateSent, mr->timeSent);
	MessageDateReceived = Message2Time(mr->dateReceived, mr->timeReceived);

	offset += MESSAGE_RECORD_HEADER_SIZE;
}

void Message::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize());
	offset += finish - (data.GetData() + offset);
}

void Message::BuildHeader(Data &data, size_t &offset) const
{
	throw std::logic_error("Message::BuildHeader not yet implemented");
}

void Message::BuildFields(Data &data, size_t &offset) const
{
	throw std::logic_error("Message::BuildFields not yet implemented");
}

void Message::Clear()
{
	From.clear();
	To.clear();
	Cc.clear();
	Bcc.clear();
	Sender.clear();
	ReplyTo.clear();
	Subject.clear();
	Body.clear();
	Attachment.clear();
	
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

std::string Message::SimpleEmailAddress() const
{
	if( From.Email.size() ) {
		// remove all spaces from the email
		std::string ret;
		for( size_t i = 0; i < From.Email.size(); i++ )
			if( From.Email[i] != ' ' )
				ret += From.Email[i];

		return ret;
	}
	else {
		return "unknown";
	}
}

// dump message in mbox format
void Message::Dump(std::ostream &os) const
{
	static const char *MessageImportance[] = 
		{ "Low", "Normal", "High", "Unknown Priority" };
	static const char *MessageSensitivityString[] = 
		{ "Normal", "Personal", "Private", "Confidential", "Unknown Sensivity" };
	
	os << "From " << SimpleEmailAddress() << "  " << ctime( &MessageDateReceived );
	os << "X-Record-ID: (" << setw(8) << std::hex << MessageRecordId << ")\n";
	if( MessageReplyTo )
		os << "X-rim-org-msg-ref-id: " << std::dec << MessageReplyTo << "\n";
	if( MessageSaved )
		os << "X-Message-Status: Saved\n";
	else if( MessageRead )
		os << "Message Status: Opened\n";
	if( MessagePriority != NormalPriority )
		os << "Importance: " << MessageImportance[MessagePriority] << "\n";
	if( MessageSensitivity != NormalSensitivity )
		os << "Sensitivity: " << MessageSensitivityString[MessageSensitivity] << "\n";
	os << "Date: " << ctime(&MessageDateSent);
	os << "From: " << From << "\n";
	if( To.Email.size() )
		os << "To: " << To << "\n";
	if( Cc.Email.size() )
		os << "Cc: " << Cc << "\n";
	if( Bcc.Email.size() )
		os << "Bcc: " << Bcc << "\n";
	if( Sender.Email.size() )
		os << "Sender: " << Sender << "\n";
	if( ReplyTo.Email.size())
		os << "Reply To: " << ReplyTo << "\n";
	if( Subject.size() )
		os << "Subject: " << Subject << "\n";
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
	if( Attachment.size() )
		os << "Attachments: " << Attachment << "\n";
	
	os << Unknowns;
	os << "\n\n";
}


} // namespace Barry

