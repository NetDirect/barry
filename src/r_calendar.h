///
/// \file	r_calendar.h
///		Blackberry database record parser class for calndar records.
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_CALENDAR_H__
#define __BARRY_RECORD_CALENDAR_H__

#include "dll.h"
#include "record.h"
#include "r_recur_base.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//



/// \addtogroup RecordParserClasses
/// @{

class BXEXPORT Calendar : public RecurBase
{
public:
	typedef std::vector<UnknownField>		UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	// general data
	bool AllDayEvent;
	std::string Subject;
	std::string Notes;
	std::string Location;
	time_t NotificationTime;	// 0 means notification is off
	time_t StartTime;
	time_t EndTime;
	EmailAddressList Organizer;
	EmailAddressList AcceptedBy;
	EmailAddressList Invited;		// list of invited people (email a

	///
	/// Free Busy Flag
	///
	/// This lists the available settings found in the device.
	/// This list is based on information from MS Outlook 2007
	/// (Free ==0 and Busy == 2)
	/// This is FBTYPE in RFC2445 and is defined as
	/// FREE, BUSY, BUSY-UNAVAILABLE and BUSY-TENTATIVE
	///
	enum FreeBusyFlagType {
		Free = 0,
		Tentative,
		Busy,
		OutOfOffice
	};
	FreeBusyFlagType FreeBusyFlag;

	///
	/// Class Flag
	///
	/// This is also called classification in Evolution and it
	///  is the equivilant of public or private in outlook
	///  Private is set to 0x2 in Outlook
	///  RFC2445 CLASS is PUBLIC, PRIVATE, CONFIDENTIAL
	///
	enum ClassFlagType {
		Public = 0,
		Confidential,
		Private
	};

	ClassFlagType ClassFlag;


	unsigned short TimeZoneCode;	// the time zone originally used
					// for the recurrence data...
					// seems to have little use, but
					// set to your current time zone
					// as a good default
	bool TimeZoneValid;		// true if the record contained a
					// time zone code

	// unknown
	UnknownsType Unknowns;

protected:
	static FreeBusyFlagType FreeBusyFlagProto2Rec(uint8_t f);
	static uint8_t FreeBusyFlagRec2Proto(FreeBusyFlagType f);

	static ClassFlagType ClassFlagProto2Rec(uint8_t f);
	static uint8_t ClassFlagRec2Proto(ClassFlagType f);

public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);

public:
	Calendar();
	~Calendar();

	// Parser / Builder API (see parser.h / builder.h)
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset, const IConverter *ic = 0) const;

	void Clear();

	void Dump(std::ostream &os) const;

	// sorting
	bool operator<(const Calendar &other) const { return StartTime < other.StartTime; }

	// database name
	static const char * GetDBName() { return "Calendar"; }
	static uint8_t GetDefaultRecType() { return 5; }	// or 0?
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Calendar &msg) {
	msg.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

