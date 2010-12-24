///
/// \file	r_task.h
///		Record parsing class for the task database.
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)
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

#ifndef __BARRY_RECORD_SMS_H__
#define __BARRY_RECORD_SMS_H__

#include "dll.h"
#include "record.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

class BXEXPORT Sms
{
public:
	typedef Barry::UnknownsType			UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	enum MessageType
	{
		Unknown = 0,
		Received,
		Sent,
		Draft
	};
	MessageType MessageStatus;

	enum DeliveryType
	{
		NoReport = 0,
		Failed,
		Succedded
	};
	DeliveryType DeliveryStatus; // not implemented yet

	bool IsNew;
	bool NewConversation;
	bool Saved;
	bool Deleted;
	bool Opened;

	uint64_t Timestamp; // milliseconds from Jan 1, 1970
	uint64_t ServiceCenterTimestamp; // not applicable for non-incoming messages

	enum DataCodingSchemeType
	{
		SevenBit = 0,
		EightBit,
		UCS2
	};
	DataCodingSchemeType DataCodingScheme;

	uint32_t ErrorId;

	std::vector<std::string> Addresses;
	std::string Body;

	UnknownsType Unknowns;

public:
	Sms();
	~Sms();

	time_t GetTime() const;
	time_t GetServiceCenterTime() const;
	void SetTime(const time_t timestamp, unsigned int milliseconds = 0);
	void SetServiceCenterTime(const time_t timestamp, unsigned int milliseconds = 0);

	const unsigned char* ParseField(const unsigned char *begin, const unsigned char *end, const IConverter *ic = 0);
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;

	void Clear();

	void Dump(std::ostream &os) const;

	static std::string ConvertGsmToUtf8(const std::string &);

	// sorting
	bool operator<(const Sms &other) const {
		return Timestamp < other.Timestamp;
	}

	// database name
	static const char * GetDBName() { return "SMS Messages"; }
	static uint8_t GetDefaultRecType() { return 5; }
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Sms &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif

