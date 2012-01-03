///
/// \file	r_calllog.h
///		Record parsing class for call logs
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_CALLLOG_H__
#define __BARRY_RECORD_CALLLOG_H__

#include "dll.h"
#include "record.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

class BXEXPORT CallLog
{
public:
	typedef Barry::UnknownsType		UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	uint32_t Duration;		//< duration of call in seconds
	uint64_t Timestamp;		//< timestamp of call in milliseconds
					//< use GetTime() to convert to
					//< a time_t
	std::string ContactName;
	std::string PhoneNumber;

	enum DirectionFlagType
	{
		Receiver = 0,		//< We have received a call
		Emitter,		//< We have composed a call
		Failed,			//< We have missing a call and the
					//< emitter is arrived on our
					//< answering machine
		Missing			//< We have missing a call. The
					//< emitter has stopped the
					//< communication.
	};
	DirectionFlagType DirectionFlag;

	enum StatusFlagType
	{
		OK = 0,			//
		Busy,			//< We have sent the emitter on our
					//< answering machine
		NetError,		//< Network communication error
		Unknown			//< Not supported by Barry CallLog parser
	};
	StatusFlagType StatusFlag;

	enum PhoneTypeFlagType
	{
		TypeUndefined = 0,
		TypeOffice,
		TypeHome,
		TypeMobile,
		TypeUnknown		// To be completed (Office2, Home2, Pager ???)
	};
	PhoneTypeFlagType PhoneTypeFlag;

	enum PhoneInfoFlagType
	{
		InfoUndefined = 0,
		InfoKnown,		//< PhoneNumber should be set
		InfoUnknown,		//< PhoneNumber isn't set
		InfoPrivate		//< PhoneNumber is private
	};
	PhoneInfoFlagType PhoneInfoFlag;

	UnknownsType Unknowns;

protected:
	time_t GetTime() const;

	static DirectionFlagType DirectionProto2Rec(uint8_t s);
	static uint8_t DirectionRec2Proto(DirectionFlagType s);

	static PhoneTypeFlagType PhoneTypeProto2Rec(uint8_t s);
	static uint8_t PhoneTypeRec2Proto(PhoneTypeFlagType s);

public:
	CallLog();
	~CallLog();

	// Parser / Builder API (see parser.h / builder.h)
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset, const IConverter *ic = 0) const;

	// operations (common among record classes)
	void Clear();
	void Dump(std::ostream &os) const;
	std::string GetDescription() const;

	// Sorting
	bool operator<(const CallLog &other) const { return Timestamp < other.Timestamp; }

	// database name
	static const char * GetDBName() { return "Phone Call Logs"; }
	static uint8_t GetDefaultRecType() { return 0; }
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const CallLog &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif

