///
/// \file	r_task.h
///		Record parsing class for the task database.
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
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
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

class BXEXPORT Task
{
public:
	typedef std::vector<UnknownField>			UnknownsType;
	uint8_t RecType;
	uint32_t RecordId;

	uint8_t TaskType;
	std::string Summary;
	std::string Notes;
	std::string Categories;
	std::string UID;

	time_t StartTime;
	time_t DueTime;
	time_t AlarmTime;
	int TimeZoneCode;

	enum AlarmFlagType
	{
		Date = 1,
		Relative
	};
	AlarmFlagType AlarmType;

	unsigned short Interval;
	enum RecurringCodeType {
		Day = 1,		//< eg. every day
					//< set: nothing
		MonthByDate = 3,	//< eg. every month on the 12th
					//< set: DayOfMonth
		MonthByDay = 4,		//< eg. every month on 3rd Wed
					//< set: DayOfWeek and WeekOfMonth
		YearByDate = 5,		//< eg. every year on March 5
					//< set: DayOfMonth and MonthOfYear
		YearByDay = 6,		//< eg. every year on 3rd Wed of Jan
					//< set: DayOfWeek, WeekOfMonth, and
					//<      MonthOfYear
		Week = 12		//< eg. every week on Mon and Fri
					//< set: WeekDays
	};
	RecurringCodeType RecurringType;
	time_t RecurringEndTime;	
	unsigned short			// recurring details, depending on type
		DayOfWeek,		// 0-6
		WeekOfMonth,		// 1-5
		DayOfMonth,		// 1-31
		MonthOfYear;		// 1-12
	unsigned char WeekDays;		// bitmask, bit 0 = sunday

	int ClassType;
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

	bool Recurring;
	bool Perpetual;
	bool DueDateFlag;	// true if due date is set

	UnknownsType Unknowns;

public:	
	Task();
	~Task();

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

	void Clear();

	void Dump(std::ostream &os) const;
	bool operator<(const Task &other) const { return Summary < other.Summary; }

	// database name
	static const char * GetDBName() { return "Tasks"; }
	static uint8_t GetDefaultRecType() { return 2; }

};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Task &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif

