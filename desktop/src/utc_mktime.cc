///
/// \file	utc_mktime.cc
///		Utility functions for dealing with UTC->time_t conversions
///		and ISO timestamp strings.
///

/*
    Copyright (C) 2010, Chris Frey <cdfrey@foursquare.net>
    Released to the public domain.
*/

#include "utc_mktime.h"
#include <string.h>
#include <stdio.h>

bool iso_timestamp_to_tm(const char *timestamp, struct tm *result)
{
	memset(result, 0, sizeof(struct tm));
	result->tm_isdst = -1;

	int found = sscanf(timestamp, "%04d%02d%02dT%02d%02d%02d",
		&(result->tm_year), &(result->tm_mon), &(result->tm_mday),
		&(result->tm_hour), &(result->tm_min), &(result->tm_sec));

	result->tm_year -= 1900;
	result->tm_mon -= 1;
	result->tm_isdst = -1;

	return found == 6;
}

// standard mktime converts a struct tm in localtime to time_t.
// this version converts a struct tm in utc to time_t.
time_t utc_mktime(struct tm *utctime)
{
	time_t result;
	struct tm tmp, check;

	// loop, converting "local time" to time_t and back to utc tm,
	// and adjusting until there are no differences... this
	// automatically takes care of DST issues.

	// do first conversion
	tmp = *utctime;
	tmp.tm_isdst = -1;
	result = mktime(&tmp);
	if( result == (time_t)-1 )
		return (time_t)-1;
	if( gmtime_r(&result, &check) == NULL )
		return (time_t)-1;

	// loop until match
	while(  check.tm_year != utctime->tm_year ||
		check.tm_mon != utctime->tm_mon ||
		check.tm_mday != utctime->tm_mday ||
		check.tm_hour != utctime->tm_hour ||
		check.tm_min != utctime->tm_min )
	{
		tmp.tm_min  += utctime->tm_min - check.tm_min;
		tmp.tm_hour += utctime->tm_hour - check.tm_hour;
		tmp.tm_mday += utctime->tm_mday - check.tm_mday;
		tmp.tm_year += utctime->tm_year - check.tm_year;
		tmp.tm_isdst = -1;

		result = mktime(&tmp);
		if( result == (time_t)-1 )
			return (time_t)-1;
		gmtime_r(&result, &check);
		if( gmtime_r(&result, &check) == NULL )
			return (time_t)-1;
	}

	return result; 
}

