///
/// \file	r_memo.h
///		Record parsing class for the memo database.
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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
	typedef std::vector<UnknownField>       UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	uint8_t CallLogType;
	time_t Time;
	std::string Name;
	std::string Phone;

	enum StatusFlagType
	{
		Received = 0,
		Sent,
		Failed
	};
	StatusFlagType StatusFlag;

	UnknownsType Unknowns;

protected:
	static StatusFlagType StatusProto2Rec(uint8_t s);
	static uint8_t StatusRec2Proto(StatusFlagType s);

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

	void Clear();

	void Dump(std::ostream &os) const;

	// Sorting
	bool operator<(const CallLog &other) const { return Time < other.Time; }

	// database name
	static const char * GetDBName() { return "Phone Call Logs"; }
	static uint8_t GetDefaultRecType() { return 0; }    // or 0?
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const CallLog &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif

