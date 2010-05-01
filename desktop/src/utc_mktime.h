///
/// \file	utc_mktime.h
///		Utility functions for dealing with UTC->time_t conversions
///		and ISO timestamp strings.
///

/*
    Copyright (C) 2010, Chris Frey <cdfrey@foursquare.net>
    Released to the public domain.
*/

#ifndef __UTC_MKTIME_H__
#define __UTC_MKTIME_H__

#include <time.h>

bool iso_timestamp_to_tm(const char *timestamp, struct tm *result);

// standard mktime converts a struct tm in localtime to time_t.
// this version converts a struct tm in utc to time_t.
time_t utc_mktime(struct tm *utctime);

#endif

