///
/// \file	time.cc
///		Conversion between time_t and cenmin_t and back.
///		time_t is the POSIX time, seconds from Jan 1, 1970
///		min1900_t is the minutes from Jan 1, 1900
///

/*
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

#include "time.h"
#include "endian.h"
#include "debug.h"

namespace Barry {

TimeZone Zones[] = {
	{ 0x0000,  -12,   0, "Eniwetok, Kwajalein (-12)" },
	{ 0x0001,  -12,   0, "Midway Island, Samoa (-12)" },
	{ 0x0002,  -10,   0, "Hawaii (-10)" },
	{ 0x0003,   -9,   0, "Alaska (-9)" },
	{ 0x0004,   -8,   0, "Pacific Time (US & Canada), Tijuana (-8)" },
	{ 0x000a,   -7,   0, "Mountain Time (US & Canada) (-7)" },
	{ 0x000f,   -7,   0, "Arizona (-7)" },
	{ 0x000d,   -7,   0, "Chihuahua, La Paz, Mazatlan (-7)" },
	{ 0x0014,   -6,   0, "Central Time (US & Canada) (-6)" },
	{ 0x0021,   -6,   0, "Central America (-6)" },
	{ 0x0019,   -6,   0, "Saskatchewan (-6)" },
	{ 0x001e,   -6,   0, "Mexico City (-6)" },
	{ 0x0023,   -5,   0, "Eastern Time (US & Canada) (-5)" },
	{ 0x002d,   -5,   0, "Bogota, Lima, Quito (-5)" },
	{ 0x0028,   -5,   0, "Indiana (East) (-5)" },
	{ 0x0032,   -4,   0, "Atlantic Time (Canada) (-4)" },
	{ 0x0037,   -4,   0, "Caracas, La Paz (-4)" },
	{ 0x0038,   -4,   0, "Santiago (-4)" },
	{ 0x003c,   -3, -30, "Newfoundland (-3.5)" },
	{ 0x0046,   -3,   0, "Buenos Aires, Georgetown (-3)" },
	{ 0x0041,   -3,   0, "Brasilia (-3)" },
	{ 0x0049,   -3,   0, "Greenland (-3)" },
	{ 0x004b,   -2,   0, "Mid-Atlantic (-2)" },
	{ 0x0053,   -1,   0, "Cape Verde Island (-1)" },
	{ 0x0050,   -1,   0, "Azores (-1)" },
	{ 0x0055,    0,   0, "Dublin, Edinburgh, Lisbon, London (GMT)" },
	{ 0x005a,    0,   0, "Casablanca, Monrovia (GMT)" },
	{ 0x006e,    1,   0, "Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna (+1)" },
	{ 0x0071,    1,   0, "West Central Africa (+1)" },
	{ 0x005f,    1,   0, "Belgrade, Bratislava, Budapest, Ljubljana, Prague (+1)" },
	{ 0x0069,    1,   0, "Brussels, Copenhagen, Madrid, Paris (+1)" },
	{ 0x0064,    1,   0, "Sarajevo, Skopje, Sofija, Vilnius, Warsaw, Zagreb (+1)" },
	{ 0x008c,    2,   0, "Harare, Pretoria (+2)" },
	{ 0x0087,    2,   0, "Jerusalem (+2)" },
	{ 0x0073,    2,   0, "Bucharest (+2)" },
	{ 0x0078,    2,   0, "Cairo (+2)" },
	{ 0x0082,    2,   0, "Athens, Istanbul, Minsk (+2)" },
	{ 0x007d,    2,   0, "Helsinki, Riga, Tallinn (+2)" },
	{ 0x0096,    3,   0, "Kuwait, Riyadh (+3)" },
	{ 0x009b,    3,   0, "Nairobi (+3)" },
	{ 0x009e,    3,   0, "Baghdad (+3)" },
	{ 0x0091,    3,   0, "Moscow, St. Petersburg, Volgograd (+3)" },
	{ 0x00a0,    3,  30, "Tehran (+3.5)" },
	{ 0x00a5,    4,   0, "Abu Dhabi, Muscat (+4)" },
	{ 0x00aa,    4,   0, "Baku, Tbilisi, Yerevan (+4)" },
	{ 0x00af,    4,  30, "Kabul (+4.5)" },
	{ 0x00b9,    5,   0, "Islamabad, Karachi, Tashkent (+5)" },
	{ 0x00b4,    5,   0, "Ekaterinburg (+5)" },
	{ 0x00be,    5,  30, "Calcutta, Chennai, Mumbai, New Delhi (+5.5)" },
	{ 0x00c1,    5,  45, "Kathmandu (+5.75)" },
	{ 0x00c3,    6,   0, "Astana, Dhaka (+6)" },
	{ 0x00c8,    6,   0, "Sri Lanka (+6)" },
	{ 0x00c9,    6,   0, "Almaty, Novosibirsk (+6)" },
	{ 0x00cb,    6,  30, "Rangoon (+6.5)" },
	{ 0x00cd,    7,   0, "Bangkok, Hanoi, Jakarta (+7)" },
	{ 0x00cf,    7,   0, "Krasnoyarsk (+7)" },
	{ 0x00d2,    8,   0, "Beijing, Chongqing, Hong Kong, Urumqi (+8)" },
	{ 0x00d7,    8,   0, "Kuala Lumpur, Singapore (+8)" },
	{ 0x00e1,    8,   0, "Perth (+8)" },
	{ 0x00dc,    8,   0, "Taipei (+8)" },
	{ 0x00e3,    8,   0, "Irkutsk, Ulaan Bataar (+8)" },
	{ 0x00eb,    9,   0, "Osaka, Sapporo, Tokyo (+9)" },
	{ 0x00e6,    9,   0, "Seoul (+9)" },
	{ 0x00f0,    9,   0, "Yakutsk (+9)" },
	{ 0x00f5,    9,  30, "Darwin (+9.5)" },
	{ 0x00fa,    9,  30, "Adelaide (+9.5)" },
	{ 0x0104,   10,   0, "Brisbane (+10)" },
	{ 0x0113,   10,   0, "Guam, Port Moresby (+10)" },
	{ 0x00ff,   10,   0, "Canberra, Melbourne, Sydney (+10)" },
	{ 0x0109,   10,   0, "Hobart (+10)" },
	{ 0x010e,   10,   0, "Vladivostok (+10)" },
	{ 0x0118,   11,   0, "Magadan, Solomon Islands, New Caledonia (+11)" },
	{ 0x011d,   12,   0, "Fiji, Kamchatka, Marshall Islands (+12)" },
	{ 0x0122,   12,   0, "Auckland, Wellington (+12)" },
	{ 0x012c,   13,   0, "Nuku'alofa (+13)" },
	{ 0,         0,   0, 0 }
};

min1900_t time2min(time_t t)
{
	if( t == 0 )
		return htobl(0xffffffff);

	min1900_t r = t / 60 + STDC_MIN1900_DIFF;
	return htobl(r);
}

time_t min2time(min1900_t m)
{
	if( (unsigned long) btohl(m) == 0xffffffff )
		return 0;
	else
		return (btohl(m) - STDC_MIN1900_DIFF) * 60;
}


//
// GetTimeZoneTable
//
/// Returns a pointer to an array of TimeZone structs.
/// The last struct contains 0 in all fields, and can be used as
/// an "end of array" marker.
///
const TimeZone* GetTimeZoneTable()
{
	return Zones;
}

//
// GetTimeZone
//
/// Searches the internal timezone code table for the given Code
/// and returns a pointer to a TimeZone struct found.  If the
/// code is not found, a pointer to a valid TimeZone struct is
/// is still returned, but the struct's Code contains TIME_ZONE_CODE_ERR,
/// and the name is "Unknown time zone."  The unknown timezone
/// is the same offset as GMT.
///
const TimeZone* GetTimeZone(unsigned short Code)
{
	static TimeZone Unknown = { TIME_ZONE_CODE_ERR, 0, 0, "Unknown time zone" };

	for( TimeZone *z = Zones; z->Name; z++ ) {
		if( Code == z->Code )
			return z;
	}
	return &Unknown;
}

//
// GetTimeZoneCode
//
/// Searches the internal timezone table for the first matching
/// Code.  If no matching Code is found, TIME_ZONE_CODE_ERR is returned.
///
/// This function does not adjust for daylight saving time.
///
unsigned short GetTimeZoneCode(signed short HourOffset,
			       signed short MinOffset)
{
	for( TimeZone *z = Zones; z->Name; z++ ) {
		if( HourOffset == z->HourOffset && MinOffset == z->MinOffset )
			return z->Code;
	}
	return TIME_ZONE_CODE_ERR;
}

/// This routine takes the day of the year and
/// returns a time_t adjusted from the first of
/// the year.
///
/// FIXME This function assumes the year hasn't changed,
/// but I don't have enough information to determine
/// where the year is in this header info
///
time_t DayToDate( unsigned short Day )
{
	struct tm *now, then;
	time_t t = time( NULL );
	
	now = localtime( &t ); // need this to get year
	// set to Jan 1 midnight, this year;
	then.tm_sec = 0;
	then.tm_min = 0;
	then.tm_hour = 0;
	then.tm_mday = 0;
	then.tm_mon = 0;
	then.tm_year = now->tm_year;
	then.tm_isdst = -1;
	t = mktime(&then);
	t -= 60*60;			// need to subract an hour
	t += Day * 24 * 60 * 60;	// Add the day converted to seconds
	
	return t;
}

//
// Message2Time
//
/// Localize the funky math used to convert a Blackberry message
/// timestamp into a time_t.
///
/// Both r_date and r_time are expected to be fed in from the
/// Protocol::MessageRecord struct in raw form, without endian
/// conversion.  This function handles that.
///
time_t Message2Time(uint16_t r_date, uint16_t r_time)
{
	dout("Message2Time(0x" << std::hex << btohs(r_date) << ", 0x"
		<< btohs(r_time) << ")");

	time_t result = ( btohs(r_date) & 0x01ff ) - 0x29;
	result = DayToDate( result );
	result += (time_t)( btohs(r_time)*1.77 );

	dout("Message2Time result: " << ctime(&result));
	return result;
}

//
// ThreadTimeout
//
/// Creates a pthread_cond_timedwait() compatible timespec struct,
/// based on a given timeout in milliseconds.  Note that the resulting
/// timespec is time-sensitive: the 'timer' starts as soon as this function
/// returns, since timespec is a specific time in the future,
/// and ThreadTimeout() calculates it based on the current time.
///
/// Returns the spec pointer, to make it easy to use with
/// pthread_cond_timedwait()
///
BXEXPORT struct timespec* ThreadTimeout(int timeout_ms, struct timespec *spec)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	spec->tv_sec = now.tv_sec + timeout_ms / 1000;
	spec->tv_nsec = (now.tv_usec + timeout_ms % 1000 * 1000) * 1000;

	return spec;
}



} // namespace Barry


#ifdef __TEST_MODE__

#include <iostream>
#include <iomanip>

using namespace std;
using namespace Barry;

void display(const char *msg, time_t t)
{
	cout << msg << ": " << ctime(&t);
	cout << msg << " seconds: "
		<< setbase(10) << t
		<< "(0x" << setbase(16) << t << ")"
		<< endl;
	cout << msg << " minutes: "
		<< setbase(10) << (t/60)
		<< "(0x" << setbase(16) << (t/60) << ")"
		<< endl;
	cout << endl;
}

void calc(const char *msg, time_t t, min1900_t dbval)
{
	cout << msg << endl;
	display("    Initial time", t);
	display("    DB Val", min2time(dbval));
}

int main()
{
	struct tm start;
	time_t t;

	// set to Oct 4, 2005, 2pm;
	start.tm_sec = 0;
	start.tm_min = 0;
	start.tm_hour = 14;
	start.tm_mday = 4;
	start.tm_mon = 9;
	start.tm_year = 105;
	start.tm_isdst = -1;
	t = mktime(&start);
	calc("Oct 4", t, 0x0350c118);

	// comparison
	t = time(NULL);
	min1900_t m = time2min(t);
	time_t tc = min2time(m);
	cout << "Original time: " << t << endl;
	cout << "time2min:      " << m << endl;
	cout << "min2time:      " << tc << endl;
	if( t == (tc + t % 60) )
		cout << "Success! (orig == converted + mod)" << endl;
	else
		cout << "Failed!" << endl;

	// time zone
	cout << "Should say Eastern: " << GetTimeZone(0x23)->Name << endl;
	cout << "should say Unknown: " << GetTimeZone(0xffff)->Name << endl;
}

#endif

