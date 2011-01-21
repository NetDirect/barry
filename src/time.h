///
/// \file	time.h
///		Time related conversion routines.
///		time_t is the POSIX time.
///		min1900_t is the minutes from Jan 1, 1900
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_TIME_H__
#define __BARRY_TIME_H__

#include "dll.h"
#include <sys/time.h>		// for struct timespec
#include <time.h>
#include <stdint.h>

//
// Calculate the number of minutes between Jan 01, 1900 and Jan 01, 1970
//
// There are 17 leap years between 1900 and 1970
//    (1969-1900) / 4 = 17.25
//
// 1900 itself is not a leap year (not divisible by 400)
//
#define DAY_MINUTES (24 * 60)
#define YEAR_MINUTES (365 * DAY_MINUTES)
#define LEAP_YEAR_COUNT ((1970-1901) / 4)
#define YEAR_COUNT (1970 - 1900)

// therefore, the difference between standard C's time and min1900_t's
// time in minutes:
#define STDC_MIN1900_DIFF (YEAR_COUNT * YEAR_MINUTES + LEAP_YEAR_COUNT * DAY_MINUTES)

namespace Barry {

typedef long min1900_t;

BXEXPORT min1900_t time2min(time_t t);
BXEXPORT time_t min2time(min1900_t m);

// FIXME - turn TimeZone into a C typedef and wrap this in extern "C"
// so the data can be used in both C and C++ libraries
struct BXEXPORT TimeZone
{
	unsigned short Code;
	signed short HourOffset;
	signed short MinOffset;
	const char *Name;
};

// FIXME - put this somewhere for both C and C++
#define TIME_ZONE_CODE_ERR	0xffff

BXEXPORT const TimeZone* GetTimeZoneTable();
BXEXPORT const TimeZone* GetTimeZone(unsigned short Code);
BXEXPORT unsigned short GetTimeZoneCode(signed short HourOffset,
	signed short MinOffset = 0);

// Message time conversion stuff
BXEXPORT time_t DayToDate( unsigned short Day );
BXEXPORT time_t Message2Time(uint16_t r_date, uint16_t r_time);

// Thread timeout creation
BXEXPORT struct timespec* ThreadTimeout(int timeout_ms, struct timespec *spec);

} // namespace Barry

#endif

