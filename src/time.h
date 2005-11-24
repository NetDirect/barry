///
/// \file	time.h
///		Conversion between time_t and cenmin_t and back.
///		time_t is the POSIX time.
///		min1900_t is the minutes from Jan 1, 1900
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <time.h>

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

inline min1900_t time2min(time_t t)
{
	return t / 60 + STDC_MIN1900_DIFF;
}

inline time_t min2time(min1900_t m)
{
	return (m - STDC_MIN1900_DIFF) * 60;
}


} // namespace Barry

#endif

