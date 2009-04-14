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
#include "strnlen.h"
#include <ostream>
#include <iomanip>
#include <string.h>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// Sms Class

// SMS Field Codes
#define SMSFC_METADATA 0x01
#define SMSFC_ADDRESS 0x02
#define SMSFC_BODY 0x04

// SMS Field Sizes and Header Sizes
#define SMS_ADDRESS_HEADER_SIZE 0x04

#define MILLISECONDS_IN_A_SECOND 1000

time_t Sms::GetTime() const
{
	return (time_t)(Timestamp / MILLISECONDS_IN_A_SECOND);
}

time_t Sms::GetServiceCenterTime() const
{
	return (time_t)(ServiceCenterTimestamp / MILLISECONDS_IN_A_SECOND);
}

void Sms::SetTime(const time_t timestamp, const unsigned milliseconds)
{
	Timestamp = (uint64_t)timestamp * MILLISECONDS_IN_A_SECOND + milliseconds;
}

void Sms::SetServiceCenterTime(const time_t timestamp, const unsigned milliseconds)
{
	ServiceCenterTimestamp = (uint64_t)timestamp * MILLISECONDS_IN_A_SECOND + milliseconds;
}

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
	if (begin > end) // if begin==end, we are ok
		return begin;

	if (!btohs(field->size)) // if field has no size, something's up
		return begin;

	switch (field->type)
	{
		case SMSFC_METADATA:
		{
			if (btohs(field->size) < SMS_METADATA_SIZE)
				break; // size not match

			const SMSMetaData &metadata = field->u.sms_metadata;
			NewConversation = metadata.flags & SMS_FLG_NEW_CONVERSATION;
			Saved = metadata.flags & SMS_FLG_SAVED;
			Deleted = metadata.flags & SMS_FLG_DELETED;
			Opened = metadata.flags & SMS_FLG_OPENED;

			IsNew = metadata.new_flag;

			uint32_t status = btohl(metadata.status);

			switch (status)
			{
				case SMS_STA_RECEIVED:
					MessageStatus = Received;
					break;
				case SMS_STA_DRAFT:
					MessageStatus = Draft;
					break;
				default:
					MessageStatus = Sent; // consider all others as sent
			}

			ErrorId = btohl(metadata.error_id);

			Timestamp = btohll(metadata.timestamp);
			ServiceCenterTimestamp = btohll(metadata.service_center_timestamp);

			switch (metadata.dcs)
			{
				case SMS_DCS_7BIT:
					DataCodingScheme = SevenBit;
					break;
				case SMS_DCS_8BIT:
					DataCodingScheme = EightBit;
					break;
				case SMS_DCS_UCS2:
					DataCodingScheme = UCS2;
					break;
				default:
					DataCodingScheme = SevenBit; // consider all unknowns as 7bit
			}

			return begin;
		}

		case SMSFC_ADDRESS:
		{
			uint16_t length = btohs(field->size);
			if (length < SMS_ADDRESS_HEADER_SIZE + 1) // trailing '\0'
				break; // too short

			length -= SMS_ADDRESS_HEADER_SIZE;
			const char *address = (const char *)field->u.raw + SMS_ADDRESS_HEADER_SIZE;
			Addresses.push_back(std::string(address, strnlen(address, length)));
			return begin;
		}

		case SMSFC_BODY:
		{
			const char *body_begin = (const char *)field->u.raw;
			Body = std::string(body_begin, body_begin + btohs(field->size));
			if (DataCodingScheme == UCS2)
			{
				if (ic)
				{
					IConvHandle ucs2("UCS-2BE", *ic);
					Body = ic->Convert(ucs2, Body);
				}
			}
			return begin;
		}
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
	MessageStatus = Unknown;
	DeliveryStatus = NoReport;
	DataCodingScheme = SevenBit;

	IsNew = NewConversation = Saved = Deleted = Opened = false;

	Timestamp = ServiceCenterTimestamp = 0;
	ErrorId = 0;

	Addresses.clear();
	Body.clear();

	Unknowns.clear();
}

void Sms::Dump(std::ostream &os) const
{

	os << "SMS record: 0x" << setbase(16) << RecordId
		<< " (" << (unsigned int)RecType << ")\n";
	time_t t = GetTime();
	os << "\tTimestamp: " << ctime(&t);

	if (MessageStatus == Received)
	{
		t = GetServiceCenterTime();
		os << "\tService Center Timestamp: " << ctime(&t);
	}

	if (ErrorId)
		os << "\tSend Error: 0x" << setbase(16) << ErrorId << "\n";

	switch (MessageStatus)
	{
		case Received:
			os << "\tReceived From:\n";
			break;
		case Sent:
			os << "\tSent to:\n";
			break;
		case Draft:
			os << "\tDraft for:\n";
			break;
		case Unknown:
			os << "\tUnknown status for:\n";
			break;
	}

	os << "\t";
	for (std::vector<std::string>::const_iterator Iterator = Addresses.begin(); Iterator < Addresses.end(); ++Iterator)
	{
		if (Iterator != Addresses.begin())
			os << ", ";
		os << *Iterator;
	}
	os << "\n";

	if (IsNew || Opened || Saved || Deleted || NewConversation)
	{
		os << "\t";
		if (IsNew)
			os << "New ";
		if (Opened)
			os << "Opened ";
		if (Saved)
			os << "Saved ";
		if (Deleted)
			os << "Deleted ";
		os << "Message" << (NewConversation ? " that starts a new conversation" : "") << "\n";
	}
	os << "\tContent: " << Body << "\n";
	os << "\n";
}

} // namespace Barry
