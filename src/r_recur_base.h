///
/// \file	r_recur_base.h
///		Base class for recurring calendar event data.
///

/*
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_RECUR_BASE_H__
#define __BARRY_RECORD_RECUR_BASE_H__

#include "dll.h"
#include "record.h"
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

class BXEXPORT RecurBase
{
public:
	///
	/// Recurring data
	///
	/// Note: interval can be used on all of these recurring types to
	///       make it happen "every other time" or more, etc.
	///
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


	bool Recurring;
	RecurringCodeType RecurringType;
	uint16_t Interval;		// must be >= 1
	Barry::TimeT RecurringEndTime;	// only pertains if Recurring is true
					// sets the date and time when
					// recurrence of this appointment
					// should no longer occur
					// If a perpetual appointment, this
					// is 0xFFFFFFFF in the low level data
					// Instead, set the following flag.
	bool Perpetual;			// if true, this will always recur

	uint16_t			// recurring details, depending on type
		DayOfWeek,		// 0-6
		WeekOfMonth,		// 1-5
		DayOfMonth,		// 1-31
		MonthOfYear;		// 1-12
	uint8_t WeekDays;		// bitmask, bit 0 = sunday

// FIXME - put these somewhere usable by both C and C++
		#define CAL_WD_SUN	0x01
		#define CAL_WD_MON	0x02
		#define CAL_WD_TUE	0x04
		#define CAL_WD_WED	0x08
		#define CAL_WD_THU	0x10
		#define CAL_WD_FRI	0x20
		#define CAL_WD_SAT	0x40

protected:
	void ParseRecurrenceData(const void *data);
	static unsigned char WeekDayProto2Rec(uint8_t raw_field);
	static uint8_t WeekDayRec2Proto(unsigned char weekdays);

protected:
	RecurBase();
	virtual ~RecurBase();

public:
	void Validate() const;

	// return true if parse, false if not (for example, if type not
	// recognized)
	bool ParseField(uint8_t type, const unsigned char *data, size_t size,
		const IConverter *ic = 0);
	void BuildRecurrenceData(time_t StartTime, void *data) const;

	uint8_t RecurringFieldType() const;

	void Clear();

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const RecurBase &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif

