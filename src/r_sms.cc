///
/// \file	r_sms.cc
///		Record parsing class for the SMS database.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
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
#include "ios_state.h"

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
			//
			// Some SMS bodies contain a null terminator
			// in the middle, and it is unknown at the moment
			// why this is.  For regular 8bit char strings,
			// we just strip out the nulls.  For UCS2
			// 16bit char strings, we strip out the
			// 16bit nulls.
			//
			// Any further information on why these null
			// terminators appear is welcome.
			//
			const char *str = (const char *)field->u.raw;
			uint16_t maxlen = btohs(field->size);
			if (DataCodingScheme != UCS2) {
				for (uint16_t i = 0; i < maxlen; ++i) {
					if (str[i]) // if not null, push it
						Body += str[i];
				}
			}
			else {
				for (uint16_t i = 0; maxlen && i < (maxlen-1); i += 2) {
					if (str[i] || str[i + 1]) // if not null, push it
						Body += std::string(str + i, 2);
				}
			}
			if (ic) {
				if (DataCodingScheme == SevenBit) {
					// SevenBit -> UTF-8 -> ic's tocode
					IConvHandle utf8("UTF-8", *ic);
					Body = ic->Convert(utf8, ConvertGsmToUtf8(Body)); // convert the Body string from GSM 03.38 defined to UTF-8
				}
				else if (DataCodingScheme == EightBit)
					Body = ic->FromBB(Body);
				else {
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

void Sms::BuildHeader(Data &data, size_t &offset) const
{
	// not yet implemented
}

void Sms::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	// not yet implemented
}

void Sms::Clear()
{
	RecType = GetDefaultRecType();
	RecordId = 0;

	MessageStatus = Unknown;
	DeliveryStatus = NoReport;

	IsNew = NewConversation = Saved = Deleted = Opened = false;

	Timestamp = ServiceCenterTimestamp = 0;

	DataCodingScheme = SevenBit;

	ErrorId = 0;

	Addresses.clear();
	Body.clear();

	Unknowns.clear();
}

std::string Sms::GetDescription() const
{
	if( Addresses.size() )
		return Addresses[0];
	else
		return "Unknown destination";
}

void Sms::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	os << "SMS record: 0x" << setbase(16) << RecordId
		<< " (" << (unsigned int)RecType << ")\n";
	time_t t = GetTime();
	os << "\tTimestamp: " << ctime(&t);

	if (MessageStatus == Received) {
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
	for (std::vector<std::string>::const_iterator i = Addresses.begin(); i < Addresses.end(); ++i) {
		if (i != Addresses.begin())
			os << ", ";
		os << *i;
	}
	os << "\n";

	if (IsNew || Opened || Saved || Deleted || NewConversation) {
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

//
// This function helps to convert GSM 03.38 defined 7-bit
// SMS to UTF-8.
// Detailed information can be found in:
// ftp://ftp.3gpp.org/Specs/html-info/0338.htm (Official)
// http://en.wikipedia.org/wiki/SMS#GSM
//

std::string Sms::ConvertGsmToUtf8(const std::string &s)
{
	//
	// This array stores the GSM 03.38 defined encoding's
	// corresponding UTF-8 values.
	// For example: GsmTable[0] = "\x40", which refers to
	// a "@" in UTF-8 encoding.
	// The 0x1b item, leads to the extension table, using
	// the char right next to it as the index.
	// According to the official specification, when not
	// able to handle it, it should be treated simply as
	// a space, which is denoted in UTF-8 as "\x20".
	//
	static const std::string GsmTable[0x80] = {
	//  0x0,        0x1,        0x2,        0x3,        0x4,        0x5,        0x6,        0x7
		"\x40",     "\xc2\xa3", "\x24",     "\xc2\xa5", "\xc3\xa8", "\xc3\xa9", "\xc3\xb9", "\xc3\xac", // 0x00
		"\xc3\xb2", "\xc3\x87", "\x0a",     "\xc3\x98", "\xc3\xb8", "\x0d",     "\xc3\x85", "\xc3\xa5", // 0x08
		"\xce\x94", "\x5f",     "\xce\xa6", "\xce\x93", "\xce\x9b", "\xce\xa9", "\xce\xa0", "\xce\xa8", // 0x10
		"\xce\xa3", "\xce\x98", "\xce\x9e", "\x20",     "\xc3\x86", "\xc3\xa6", "\xc3\x9f", "\xc3\x89", // 0x18
		"\x20",     "\x21",     "\x22",     "\x23",     "\xc2\xa4", "\x25",     "\x26",     "\x27",     // 0x20
		"\x28",     "\x29",     "\x2a",     "\x2b",     "\x2c",     "\x2d",     "\x2e",     "\x2f",     // 0x28
		"\x30",     "\x31",     "\x32",     "\x33",     "\x34",     "\x35",     "\x36",     "\x37",     // 0x30
		"\x38",     "\x39",     "\x3a",     "\x3b",     "\x3c",     "\x3d",     "\x3e",     "\x3f",     // 0x38
		"\xc2\xa1", "\x41",     "\x42",     "\x43",     "\x44",     "\x45",     "\x46",     "\x47",     // 0x40
		"\x48",     "\x49",     "\x4a",     "\x4b",     "\x4c",     "\x4d",     "\x4e",     "\x4f",     // 0x48
		"\x50",     "\x51",     "\x52",     "\x53",     "\x54",     "\x55",     "\x56",     "\x57",     // 0x50
		"\x58",     "\x59",     "\x5a",     "\xc3\x84", "\xc3\x96", "\xc3\x91", "\xc3\x9c", "\xc2\xa7", // 0x58
		"\xc2\xbf", "\x61",     "\x62",     "\x63",     "\x64",     "\x65",     "\x66",     "\x67",     // 0x60
		"\x68",     "\x69",     "\x6a",     "\x6b",     "\x6c",     "\x6d",     "\x6e",     "\x6f",     // 0x68
		"\x70",     "\x71",     "\x72",     "\x73",     "\x74",     "\x75",     "\x76",     "\x77",     // 0x70
		"\x78",     "\x79",     "\x7a",     "\xc3\xa4", "\xc3\xb6", "\xc3\xb1", "\xc3\xbc", "\xc3\xa0"  // 0x78
	};
	//
	// This sparse array stores the GSM 03.38 defined
	// encoding extension table.
	// The \x1b item is also preserved, for possibly
	// another extension table.
	//
	static const std::string GsmExtensionTable[0x80] = {
	//  0x0,            0x1,            0x2,            0x3,            0x4,            0x5,            0x6,            0x7
		"",             "",             "",             "",             "",             "",             "",             "",     // 0x00
		"",             "",             "\x0c",         "",             "",             "",             "",             "",     // 0x08
		"",             "",             "",             "",             "\x5e",         "",             "",             "",     // 0x10
		"",             "",             "",             " ",            "",             "",             "",             "",     // 0x18
		"",             "",             "",             "",             "",             "",             "",             "",     // 0x20
		"\x7b",         "\x7d",         "",             "",             "",             "",             "",             "\x5c", // 0x28
		"",             "",             "",             "",             "",             "",             "",             "",     // 0x30
		"",             "",             "",             "",             "\x5b",         "\x7e",         "\x5d",         "",     // 0x38
		"\x7c",         "",             "",             "",             "",             "",             "",             "",     // 0x40
		"",             "",             "",             "",             "",             "",             "",             "",     // 0x48
		"",             "",             "",             "",             "",             "",             "",             "",     // 0x50
		"",             "",             "",             "",             "",             "",             "",             "",     // 0x58
		"",             "",             "",             "",             "",             "\xe2\x82\xac", "",             "",     // 0x60
		"",             "",             "",             "",             "",             "",             "",             "",     // 0x68
		"",             "",             "",             "",             "",             "",             "",             "",     // 0x70
		"",             "",             "",             "",             "",             "",             "",             ""      // 0x78
	};
	std::string ret;
	unsigned len = s.length();
	for (unsigned i = 0; i < len; ++i) {
		unsigned char c = (unsigned char) s[i];
		if (c > 0x7f) // prevent from illegal index
			continue;
		else if (c == 0x1b) { // go to extension table
			if (i < len - 1) {
				c = (unsigned char) s[++i];
				if (c <= 0x7f) // prevent from illegal index
					ret += GsmExtensionTable[c];
			}
		}
		else
			ret += GsmTable[c];
	}
	return ret;
}

} // namespace Barry

