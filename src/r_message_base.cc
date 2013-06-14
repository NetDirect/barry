///
/// \file	r_message_base.cc
///		Base class for email-oriented Blackberry database records
///

/*
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)
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

#include "i18n.h"
#include "r_message_base.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "endian.h"
#include "iconv.h"
#include <ostream>
#include <iomanip>
#include <stdexcept>
#include "ios_state.h"

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// Message class


// Email / message field codes
#define MBFC_TO			0x01	// can occur multiple times
#define MBFC_CC			0x02	// ditto
#define MBFC_BCC		0x03	// ditto
#define MBFC_SENDER		0x04
#define MBFC_FROM		0x05
#define MBFC_REPLY_TO		0x06
#define MBFC_SUBJECT		0x0b
#define MBFC_BODY		0x0c
#define MBFC_REPLY_UNKNOWN	0x12	// This shows up as 0x00 on replies
					// but we don't do much with it now
#define MBFC_ATTACHMENT		0x16
#define MBFC_RECORDID		0x4b	// Internal Message ID, mimics header RecNumber
#define MBFC_END		0xffff

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

static FieldLink<MessageBase> MessageBaseFieldLinks[] = {
   { MBFC_TO,        N_("To"),           0, 0, 0, &MessageBase::To, 0, 0, 0, true },
   { MBFC_CC,        N_("Cc"),           0, 0, 0, &MessageBase::Cc, 0, 0, 0, true },
   { MBFC_BCC,       N_("Bcc"),          0, 0, 0, &MessageBase::Bcc, 0, 0, 0, true },
   { MBFC_SENDER,    N_("Sender"),       0, 0, 0, &MessageBase::Sender, 0, 0, 0, true },
   { MBFC_FROM,      N_("From"),         0, 0, 0, &MessageBase::From, 0, 0, 0, true },
   { MBFC_REPLY_TO,  N_("ReplyTo"),      0, 0, 0, &MessageBase::ReplyTo, 0, 0, 0, true },
   { MBFC_SUBJECT,   N_("Subject"),      0, 0, &MessageBase::Subject, 0, 0, 0, 0, true },
   { MBFC_BODY,      N_("Body"),         0, 0, &MessageBase::Body, 0, 0, 0, 0, true },
   { MBFC_ATTACHMENT,N_("Attachment"),   0, 0, &MessageBase::Attachment, 0, 0, 0, 0, false },
   { MBFC_END,       N_("End of List"),  0, 0, 0, 0, 0, 0, 0, false }
};

MessageBase::MessageBase()
{
	Clear();
}

MessageBase::~MessageBase()
{
}

const unsigned char* MessageBase::ParseField(const unsigned char *begin,
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
	for(	FieldLink<MessageBase> *b = MessageBaseFieldLinks;
		b->type != MBFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				// parse regular string
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				if( b->iconvNeeded && ic )
					s = ic->FromBB(s);
				return begin;	// done!
			}
			else if( b->addrMember ) {
				// parse email address
				// get dual name+addr string first
				const char *fa = (const char*)field->u.addr.addr;
				std::string dual(fa, btohs(field->size) - sizeof(field->u.addr.unknown));

				// assign first string, using null terminator
				// letting std::string add it for us if it
				// doesn't exist
				EmailAddress a;
				a.Name = dual.c_str();

				// assign second string, using first size
				// as starting point
				a.Email = dual.c_str() + a.Name.size() + 1;

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
	switch( field->type )
	{
	case MBFC_RECORDID:
		MessageRecordId = btohl(field->u.uint32);
		return begin;

	case MBFC_REPLY_UNKNOWN:	// FIXME - not available in SavedMessage?
		//char swallow = field->u.raw[0];
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	return begin;
}

void MessageBase::ParseHeader(const Data &data, size_t &offset)
{
	const unsigned char *begin = data.GetData();
	const unsigned char *end = data.GetData() + data.GetSize();

	begin += offset + MESSAGE_RECORD_HEADER_SIZE;
	if( begin > end )
		return;

	MAKE_RECORD(const Barry::Protocol::MessageRecord, mr, data, offset);

	// Priority
	Priority = NormalPriority;

	uint16_t priority = btohs(mr->priority);  // deal with endian swap once
	if( priority & PRIORITY_MASK ) {
		if( priority & PRIORITY_HIGH ) {
			Priority = HighPriority;
		}
		else if( priority & PRIORITY_LOW ) {
			Priority = LowPriority;
		}
		else
			Priority = UnknownPriority;
	}

	// Sensitivity
	Sensitivity = NormalSensitivity;

	if( priority & SENSITIVE_MASK ) {
		if(( priority & SENSITIVE_CONFIDENTIAL ) == SENSITIVE_CONFIDENTIAL ) {
			Sensitivity = Confidential;
		}
		else if(( priority & SENSITIVE_PRIVATE ) == SENSITIVE_PRIVATE ) {
			Sensitivity = Private;
		}
		else if(( priority & SENSITIVE_PERSONAL ) == SENSITIVE_PERSONAL ) {
			Sensitivity = Personal;
		}
		else
			Sensitivity = UnknownSensitivity;
	}

	// X-rim-org-message-ref-id
	// NOTE: I'm cheating a bit here and using this as a reply-to
	// It's actually sent by BB with the actual UID in every message
	if( mr->inReplyTo )
		MessageReplyTo = btohl(mr->inReplyTo);

	// Status Flags
	uint32_t flags = btohl(mr->flags);

	// NOTE: A lot of these flags are 'backwards' but this seemed
	// like the most logical way to interpret them for now
	if( !( flags & MESSAGE_READ ))
		MessageRead = true;

	// NOTE: This is a reply, the original message's flags are not changed
	// the inReplyTo field is updated with the original messages's UID
	if(( flags & MESSAGE_REPLY ) == MESSAGE_REPLY )
		MessageReply = true;

	// NOTE: This bit is unset on truncation, around 4096 on my 7100g
	// NOTE: bit 0x400 is set on REALLY huge messages, haven't tested
	//       the exact size yet
	if( !( flags & MESSAGE_TRUNCATED ))
		MessageTruncated = true;

	// NOTE: Saved to 'saved' folder
	if( !( flags & MESSAGE_SAVED ))
		MessageSaved = true;

	// NOTE: Saved to 'saved' folder and then deleted from inbox
	if( !( flags & MESSAGE_SAVED_DELETED ))
		MessageSavedDeleted = true;

	MessageDateSent.Time = Message2Time(mr->dateSent, mr->timeSent);
	MessageDateReceived.Time = Message2Time(mr->dateReceived, mr->timeReceived);

	offset += MESSAGE_RECORD_HEADER_SIZE;
}

void MessageBase::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void MessageBase::Validate() const
{
}

void MessageBase::BuildHeader(Data &data, size_t &offset) const
{
	throw std::logic_error(_("MessageBase::BuildHeader not yet implemented"));
}

void MessageBase::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	throw std::logic_error(_("MessageBase::BuildFields not yet implemented"));
}

void MessageBase::Clear()
{
	// these must be overwritten by any derived classes
	RecType = 0;
	RecordId = 0;

	// clear base class variables
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
	MessageDateSent.clear();
	MessageDateReceived.clear();

	MessageTruncated = false;
	MessageRead = false;
	MessageReply = false;
	MessageSaved = false;
	MessageSavedDeleted = false;

	Priority = NormalPriority;
	Sensitivity = NormalSensitivity;

	Unknowns.clear();
}

template <class RecordT>
void DoFillHandles(typename FieldHandle<RecordT>::ListT &handles)
{
	// start fresh
	handles.clear();

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME handles

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME RecordT

	FHP(RecType, _("Record Type Code"));
	FHP(RecordId, _("Unique Record ID"));

	FHD(From, _("From"), MBFC_FROM, true);
	FHD(To, _("To"), MBFC_TO, true);
	FHD(Cc, _("CC"), MBFC_CC, true);
	FHD(Bcc, _("BCC"), MBFC_BCC, true);
	FHD(Sender, _("Sender"), MBFC_SENDER, true);
	FHD(ReplyTo, _("Reply To"), MBFC_REPLY_TO, true);
	FHD(Subject, _("Subject"), MBFC_SUBJECT, true);
	FHD(Body, _("Body"), MBFC_BODY, true);
	FHD(Attachment, _("Attachment"), MBFC_ATTACHMENT, false);

	FHD(MessageRecordId, _("Message Record ID"), MBFC_RECORDID, false);
	FHP(MessageReplyTo, _("Message Reply To"));
	FHP(MessageDateSent, _("Date Sent"));
	FHP(MessageDateReceived, _("Date Received"));

	FHP(MessageTruncated, _("Truncated"));
	FHP(MessageRead, _("Read"));
	FHP(MessageReply, _("Reply"));
	FHP(MessageSaved, _("Saved"));
	FHP(MessageSavedDeleted, _("Saved Deleted"));

	FHET(pt, PriorityType, Priority, _("Priority"));
	FHE_CONST(pt, LowPriority, _("Low"));
	FHE_CONST(pt, NormalPriority, _("Normal"));
	FHE_CONST(pt, HighPriority, _("High"));
	FHE_CONST(pt, UnknownPriority, _("Unknown"));

	FHET(st, SensitivityType, Sensitivity, _("Sensitivity"));
	FHE_CONST(st, NormalSensitivity, _("Normal"));
	FHE_CONST(st, Personal, _("Personal"));
	FHE_CONST(st, Private, _("Private"));
	FHE_CONST(st, Confidential, _("Confidential"));
	FHE_CONST(st, UnknownSensitivity, _("Unknown"));

	FHP(Unknowns, _("Unknown Fields"));
}

std::string MessageBase::GetDescription() const
{
	// FIXME - ponder a better description...
	return Subject;
}

std::string MessageBase::SimpleFromAddress() const
{
	if( From.size() ) {
		// remove all spaces from the email
		std::string ret;
		for( size_t i = 0; i < From[0].Email.size(); i++ )
			if( From[0].Email[i] != ' ' )
				ret += From[0].Email[i];

		return ret;
	}
	else {
		return _("unknown");
	}
}

// dump message in mbox format
void MessageBase::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	static const char *Importance[] =
		{ "Low", "Normal", "High", "Unknown Priority" };
	static const char *SensitivityString[] =
		{ "Normal", "Personal", "Private", "Confidential", "Unknown Sensivity" };

	os << "From " << SimpleFromAddress() << "  " << MessageDateReceived << "\n";

/*
FIXME
// savedmessage prints like this:
os << "From " << SimpleFromAddress() << "  " << ctime( &MessageDateSent );
// pinmessage prints like this:
os << "From " << SimpleFromAddress() << "  " << ctime( &MessageDateSent );
*/

	os << "X-Record-ID: (" << setw(8) << std::hex << MessageRecordId << ")\n";

	if( MessageReplyTo )
		os << "X-rim-org-msg-ref-id: " << std::dec << MessageReplyTo << "\n";
	if( MessageSaved )
		os << "X-Message-Status: Saved\n";
	else if( MessageRead )
		os << "Message Status: Opened\n";
	if( Priority != NormalPriority )
		os << "Importance: " << Importance[Priority] << "\n";
	if( Sensitivity != NormalSensitivity )
		os << "Sensitivity: " << SensitivityString[Sensitivity] << "\n";
	os << "Date: " << MessageDateSent << "\n";
	if( From.size() )
		os << "From: " << From[0] << "\n";
	if( To.size() )
		os << "To: " << To << "\n";
	if( Cc.size() )
		os << "Cc: " << Cc << "\n";
	if( Bcc.size() )
		os << "Bcc: " << Bcc << "\n";
	if( Sender.size() )
		os << "Sender: " << Sender << "\n";
	if( ReplyTo.size())
		os << "Reply To: " << ReplyTo << "\n";
	if( Subject.size() )
		os << "Subject: " << Subject << "\n";
	os << "\n";
	os << Cr2LfWrapper(Body);
	os << "\n";

	if( Attachment.size() )
		os << "Attachments: " << Data(Attachment.data(), Attachment.size()) << "\n";

	os << Unknowns;
	os << "\n\n";
}

bool MessageBase::operator<(const MessageBase &other) const
{
	// just in case either of these are set to '0', use the
	// one with the max value... this uses the latest date, which
	// is likely the most accurate
	time_t date = std::max(MessageDateSent.Time, MessageDateReceived.Time);
	time_t odate = std::max(other.MessageDateSent.Time, other.MessageDateReceived.Time);

	if( date != odate )
		return date < odate;

	return Subject < other.Subject;
}

} // namespace Barry





//////////////////////////////////////////////////////////////////////////////
// Generic Field Handle support

#include "r_message.h"
#include "r_pin_message.h"
#include "r_saved_message.h"


namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// Message class - statics

const FieldHandle<Message>::ListT& Message::GetFieldHandles()
{
	static FieldHandle<Message>::ListT fhv;

	if( fhv.size() )
		return fhv;

	DoFillHandles<Message>(fhv);
	return fhv;
}

//////////////////////////////////////////////////////////////////////////////
// PINMessage class - statics

const FieldHandle<PINMessage>::ListT& PINMessage::GetFieldHandles()
{
	static FieldHandle<PINMessage>::ListT fhv;

	if( fhv.size() )
		return fhv;

	DoFillHandles<PINMessage>(fhv);
	return fhv;
}

//////////////////////////////////////////////////////////////////////////////
// SavedMessage class - statics

const FieldHandle<SavedMessage>::ListT& SavedMessage::GetFieldHandles()
{
	static FieldHandle<SavedMessage>::ListT fhv;

	if( fhv.size() )
		return fhv;

	DoFillHandles<SavedMessage>(fhv);
	return fhv;
}

} // namespace Barry

