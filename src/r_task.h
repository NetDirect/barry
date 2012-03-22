///
/// \file	r_task.h
///		Record parsing class for the task database.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2007, Brian Edginton

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

#ifndef __BARRY_RECORD_TASK_H__
#define __BARRY_RECORD_TASK_H__

#include "dll.h"
#include "record.h"
#include "r_recur_base.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

class BXEXPORT Task : public RecurBase
{
public:
	typedef Barry::UnknownsType			UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	std::string Summary;
	std::string Notes;
	CategoryList Categories;
	std::string UID;

	Barry::TimeT StartTime;		// most devices set this the same as
					// DueTime
	Barry::TimeT DueTime;
	Barry::TimeT AlarmTime;
	uint16_t TimeZoneCode;
	bool TimeZoneValid;		// true if the record contained a
					// time zone code

	enum AlarmFlagType
	{
		Date = 1,
		Relative
	};
	AlarmFlagType AlarmType;

	enum PriorityFlagType
	{
		High = 0,
		Normal,
		Low
	};
	PriorityFlagType PriorityFlag;

	enum StatusFlagType
	{
		NotStarted = 0,
		InProgress,
		Completed,
		Waiting,
		Deferred
	};
	StatusFlagType StatusFlag;

	bool DueDateFlag;	// true if due date is set
	bool AlarmFlag;		// true if alarm/reminder is set

	// unknown
	UnknownsType Unknowns;

protected:
	static AlarmFlagType AlarmProto2Rec(uint8_t a);
	static uint8_t AlarmRec2Proto(AlarmFlagType a);

	static PriorityFlagType PriorityProto2Rec(uint8_t p);
	static uint8_t PriorityRec2Proto(PriorityFlagType p);

	static StatusFlagType StatusProto2Rec(uint8_t s);
	static uint8_t StatusRec2Proto(StatusFlagType s);

public:
	Task();
	~Task();

	// Parser / Builder API (see parser.h / builder.h)
	void Validate() const;
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

	bool operator<(const Task &other) const;

	// database name
	static const char * GetDBName() { return "Tasks"; }
	static uint8_t GetDefaultRecType() { return 2; }

	// Generic Field Handle support
	static const FieldHandle<Task>::ListT& GetFieldHandles();
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Task &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif

