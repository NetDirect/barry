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

class BXEXPORT Timezone
{
public:
	typedef Barry::UnknownsType			UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	uint8_t TZType;
	uint32_t DSTOffset;
	int32_t Index;
	int32_t Offset;
	int32_t OffsetFraction;
	uint32_t StartMonth;
	uint32_t EndMonth;
	bool Left;
	bool UseDST;

	std::string TimeZoneName;

	UnknownsType Unknowns;

protected:
	static std::vector<FieldHandle<Timezone> > m_FieldHandles;

protected:
	static void FillHandles();

public:

	Timezone();
	virtual ~Timezone();

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

	bool operator<(const Timezone &other) const { return TimeZoneName < other.TimeZoneName; }

	// database name
	static const char * GetDBName() { return "Time Zones"; }
	static uint8_t GetDefaultRecType() { return 2; }

	// Generic Field Handle support
	static const std::vector<FieldHandle<Timezone> >& GetFieldHandles()
	{
		if( !m_FieldHandles.size() )
			FillHandles();
		return m_FieldHandles;
	}
	static void ClearFieldHandles() { m_FieldHandles.clear(); }
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Timezone &msg) {
	msg.Dump(os);
	return os;
}
} // namespace Barry

#endif /* __BARRY_RECORD_TIMEZONE_H__*/
