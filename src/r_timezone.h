///
/// \file	r_timezone.h
///		Record parsing class for the timezone database.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2008, Brian Edginton

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

#ifndef __BARRY_RECORD_TIMEZONE_H__
#define __BARRY_RECORD_TIMEZONE_H__

#include "dll.h"
#include "record.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

class BXEXPORT TimeZone
{
public:
	typedef Barry::UnknownsType			UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	std::string Name;		//< name of time zone

	int32_t Index;			//< index of entry in time zone table...
					//< matches Code in hard coded TimeZone
					//< table in Barry

	int32_t UTCOffset;		//< Timezone offset from UTC in minutes.
					//< Will be a negative value for west
					//< of UTC (North America, etc), and
					//< a positive value for east (Europe).
					//< i.e. -210 for St. John's, which is
					//< -3.5 hours from UTC.

	bool UseDST;			//< true this timezone uses DST
	uint32_t DSTOffset;		//< minutes of DST, if UseDST is true.
					//< This value will almost always be 60.
	uint32_t StartMonth;		//< index, 0-11, of month to start DST
	uint32_t EndMonth;		//< index, 0-11, of month to end DST

	uint8_t TZType;			//< unknown

	UnknownsType Unknowns;

public:
	TimeZone();

	/// Creates a new timezone based on utc_offset minutes.
	/// Use same semantics as UTCOffset.  For example, a -3.5 hour
	/// timezone (which is west of UTC) would be constructed
	/// as: TimeZone(-210)
	explicit TimeZone(int utc_offset);

	/// Creates a new timezone based on negative/positive hours,
	/// and positive minutes.  For example, a -3.5 hour timezone
	/// (which is west of UTC) would be constructed as: TimeZone(-3, 30)
	/// Note that minutes can be negative, and it will be handled
	/// correctly.  i.e. TimeZone(-3, 30) == TimeZone(-3, -30)
	TimeZone(int hours, int minutes);

	virtual ~TimeZone();

	//
	// TimeZone related utility functions
	//

	bool IsWest() const { return UTCOffset < 0; }

	/// Splits UTCOffset minutes into hours and minutes.  hours can be
	/// negative.  minutes is always positive.
	void Split(int *hours, int *minutes) const;

	/// Splits UTCOffset minutes into absolute values of hours and minutes,
	/// and sets the west flag appropriately.  This is to mimic the
	/// old behaviour of the Left, Offset and OffsetFraction member
	/// variables.
	void SplitAbsolute(bool *west,
		unsigned int *hours, unsigned int *minutes) const;

	/// Creates a timezone string suitable for a Unix / POSIX TZ
	/// environment variable.  Expects a time zone prefix.
	/// For example, New Zealand Standard/Daylight Time is NZST/NZDT, so
	/// the prefix would be "NZ".  Eastern Standard/Daylight Time
	/// is EST/EDT, so the prefix would be "E".
	///
	/// Should be able to use this string to achieve time zone conversions
	/// using the TzWrapper class.
	std::string GetTz(const std::string &prefix) const;


	// common Barry record functions
	void Validate() const;
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);
	void ParseRecurrenceData(const void *data);
	void BuildRecurrenceData(void *data);
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

	bool operator<(const TimeZone &other) const { return SortByName(*this, other); }

	// sort options - suitable for use in std::sort()
	static bool SortByName(const TimeZone &a, const TimeZone &b)
	{
		return a.Name < b.Name;
	}
	static bool SortByZone(const TimeZone &a, const TimeZone &b)
	{
		return a.UTCOffset < b.UTCOffset;
	}

	// database name
	static const char * GetDBName() { return "Time Zones"; }
	static uint8_t GetDefaultRecType() { return 2; }

	// Generic Field Handle support
	static const FieldHandle<TimeZone>::ListT& GetFieldHandles();
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const TimeZone &msg) {
	msg.Dump(os);
	return os;
}


// forward declarations
namespace Mode {
	class Desktop;
}

//
// TimeZones
//
/// Creates a vector of TimeZone objects either based on the library's
/// hard coded StaticTimeZone list, or by extracting the time zone database
/// from a given device.
///
/// After construction, the vector will be sorted according to time zone,
/// with west-most first.
///
class BXEXPORT TimeZones
{
public:
	typedef std::vector<TimeZone>			ListType;
	typedef ListType::iterator			iterator;
	typedef ListType::const_iterator		const_iterator;

private:
	ListType m_list;

public:
	/// Creates the list based on the library's hard coded StaticTimeZone
	/// list.
	TimeZones();

	/// Extracts the time zone database from the given device.
	explicit TimeZones(Barry::Mode::Desktop &desktop);

	ListType& GetList() { return m_list; }
	const ListType& GetList() const { return m_list; }

	iterator begin() { return m_list.begin(); }
	const_iterator begin() const { return m_list.begin(); }

	iterator end() { return m_list.end(); }
	const_iterator end() const { return m_list.end(); }

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const TimeZones &l)
{
	l.Dump(os);
	return os;
}

} // namespace Barry

#endif /* __BARRY_RECORD_TIMEZONE_H__*/

