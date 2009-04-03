///
/// \file	r_sms.cc
///		Record parsing class for the SMS database.
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2009, Ryan Li(ryan@ryanium.com)

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

#include "r_sms.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "debug.h"
#include "iconv.h"
#include <ostream>
#include <iomanip>
#include <string.h>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// SMS Class

// SMS Field Codes
#define SMSFC_METADATA 0x01
#define SMSFC_PHONE_NUMBER 0x02
#define SMSFC_CONTENT 0x04
#define SMSFC_END 0xffff

static FieldLink<Sms> SMSFieldLinks[] = {
	{SMSFC_CONTENT, "Content", 0, 0, &Sms::Content, 0, 0, 0, 0, true},
	{SMSFC_END, "End of List", 0, 0, 0, 0, 0, 0, 0, false},
};

Sms::Sms()
{
	Clear();
}

Sms::~Sms()
{
}

const unsigned char* Sms::ParseField(const unsigned char *begin,
				     const unsigned char *end,
				     const IConverter *ic)
{
	const CommonField *field = (const CommonField *)begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if (begin > end)       // if begin==end, we are ok
		return begin;

	if (!btohs(field->size))   // if field has no size, something's up
		return begin;

	// cycle through the type table
	for(FieldLink<Sms> *b = SMSFieldLinks;
		b->type != SMSFC_END;
		b++)
	{
		if (b->type == field->type) {
			if (b->strMember) {
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				if (b->iconvNeeded && ic)
					s = ic->FromBB(s);
				return begin;   // done!
			}
		}
	}

	// handle special cases
	switch (field->type)
	{
		case SMSFC_METADATA:
		{
			if( btohs(field->size) < 30 )
				break;	// not enough data

			const unsigned char *str = (const unsigned char *)field->u.raw;
			NewConversation = str[1] & 0x20;
			Saved = str[1] & 0x10;
			Deleted = str[1] & 0x08;
			Opened = str[1] & 0x01;
			IsNew = str[2];
			if (*((uint32_t *) (str + 5)) == 0x000007ff)
				MessageStatus = Received;
			else if (*((uint32_t *) (str + 5)) == 0x7fffffff)
				MessageStatus = Draft;
			else
				MessageStatus = Sent; //consider all others as sent.
			
			ErrorId = *((uint32_t *) (str + 9));
			
			Timestamp = *((uint64_t *) (str + 13));
			
			SentTimestamp += *((uint64_t *) (str + 21));
			
			Encoding = (EncodingType)str[29];
			
			return begin;
		}

		case SMSFC_PHONE_NUMBER:
			const char *str = (const char *)field->u.raw;
			uint16_t len = btohs(field->size);
			if( len >= 4 )
				PhoneNumbers.push_back(std::string(str + 4, len - 4));
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

void Sms::ParseHeader(const Data &data, size_t &offset)
{
	// no header in SMS records
}

void Sms::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void Sms::Clear()
{
	MessageStatus = Received;
	DeliveryStatus = NoReport;
	Encoding = SevenBit;
	
	IsNew = NewConversation = Saved = Deleted = Opened = false;
	
	Timestamp = SentTimestamp = 0;
	ErrorId = 0;
	
	PhoneNumbers.clear();
	Content.clear();
	
	Unknowns.clear();
}

void Sms::Dump(std::ostream &os) const
{

	os << "SMS record: 0x" << setbase(16) << RecordId
		<< " (" << (unsigned int)RecType << ")\n";
	time_t t = Timestamp / 1000;
	os << "   Time: " << ctime(&t);

	if (MessageStatus == Received)
	{
		t = SentTimestamp / 1000;
		os << "   Sent at: " << ctime(&t);
		// FIXME - since the ISP may use a time zone other than UTC, this time probably has an offset.
	}

	if (ErrorId)
		os << "   Send Error: 0x" << setbase(16) << ErrorId << "\n";

	switch (MessageStatus)
	{
		case Received:
			os << "   Received From:" << "\n";
			break;
		case Sent:
			os << "   Sent to:" << "\n";
			break;
		case Draft:
			os << "   Draft for:" << "\n";
			break;
	}

	for (std::vector<std::string>::const_iterator Iterator = PhoneNumbers.begin(); Iterator < PhoneNumbers.end(); ++Iterator)
		os << "      " << *Iterator << "\n";

	os << "   ";
	if (IsNew)
		os << "New ";
	if (Opened)
		os << "Opened ";
	if (Saved)
		os << "Saved ";
	if (Deleted)
		os << "Deleted ";
	if (IsNew || Opened || Saved || Deleted || NewConversation)
		os << "Message" << (NewConversation ? " that starts a new conversation" : "") << "\n";
	os << "   Content: " << Content << "\n";
	os << "\n";
}

} // namespace Barry

