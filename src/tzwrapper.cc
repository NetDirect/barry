///
/// \file	tzwrapper.cc
///		Timezone adjustment class, wrapping the TZ environment
///		variable to make struct tm -> time_t conversions easier.
///

/*
    Copyright (C) 2010-2013, Chris Frey <cdfrey@foursquare.net>, To God be the glory
    Released to the public domain.
    Included in Barry and Barrified the namespace July 2010
*/

#include "tzwrapper.h"
#include <string.h>
#include <stdio.h>
#include <string>

using namespace std;

namespace Barry { namespace Sync {

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

struct tm* iso_to_tm(const char *timestamp,
			struct tm *result,
			bool &utc,
			bool *zone,
			int *zoneminutes)
{
	memset(result, 0, sizeof(struct tm));

	// handle YYYY-MM-DDTHH:MM:SS.uuu-HH:MM format
	// by stripping out the dashes and colons
	string ts = timestamp;
	string::iterator i = ts.begin();
	bool date = true;
	while( i != ts.end() ) {
		if( *i == 'T' )
			date = false;
		if( (date && *i == '-') || *i == ':' )
			ts.erase(i);
		else
			++i;
	}

	int found = sscanf(ts.c_str(), "%04d%02d%02dT%02d%02d%02d",
		&(result->tm_year), &(result->tm_mon), &(result->tm_mday),
		&(result->tm_hour), &(result->tm_min), &(result->tm_sec));

	result->tm_year -= 1900;
	result->tm_mon -= 1;
	result->tm_isdst = -1;
	if( found == 3 ) {
		// only a date available, so force time to 00:00:00
		result->tm_hour = 0;
		result->tm_min = 0;
		result->tm_sec = 0;
	}
	else if( found != 6 ) {
		return 0;
	}

	utc = ts.find('Z', 15) != string::npos;

	if( zone && zoneminutes ) {
		*zone = false;

		size_t neg = ts.find('-', 15);
		size_t pos = ts.find('+', 15);

		if( neg != string::npos || pos != string::npos ) {
			// capture timezone offset
			size_t it = neg != string::npos ? neg : pos;
			it++;
			string offset = ts.substr(it);

			int hour, min;
			found = sscanf(offset.c_str(), "%02d%02d",
				&hour, &min);
			if( offset.size() == 4 && found == 2 ) {
				*zone = true;
				*zoneminutes = hour * 60 + min;
				if( neg != string::npos )
					*zoneminutes *= -1;
			}
		}
	}

	return result;
}

std::string tm_to_iso(const struct tm *t, bool utc)
{
	char tmp[128];

	int cc = snprintf(tmp, sizeof(tmp), "%04d%02d%02dT%02d%02d%02d",
		t->tm_year + 1900,
		t->tm_mon + 1,
		t->tm_mday,
		t->tm_hour,
		t->tm_min,
		t->tm_sec
		);
	if( cc < 0 || (size_t)cc >= sizeof(tmp) )
		return "";

	if( utc ) {
		if( (size_t)cc >= (sizeof(tmp) - 1) )
			return "";		// not enough room for Z
		tmp[cc++] = 'Z';
		tmp[cc] = 0;
	}

	return tmp;
}

TzWrapper& TzWrapper::SetOffset(int zoneminutes)
{
	//
	// Set a custom TZ with the offset in hours/minutes.
	//
	// Note that TZ sees negative offsets as *ahead* of
	// UTC and positive offsets as behind UTC.  Therefore,
	// Berlin, one hour ahead of UTC is -01:00 and
	// Canada/Eastern standard time is +05:00.
	//
	// This is exactly opposite to the ISO timestamp format
	// which would have +01:00 and -05:00 respectively,
	// and therefore exactly opposite to the sign of zoneminutes.
	//
	// We use a fake timezone name of XXX here, since it
	// doesn't matter, we are only interested in the offset.

	char buf[128];
	sprintf(buf, "XXX%c%02d:%02d",
		(zoneminutes < 0 ? '+' : '-'),
		abs(zoneminutes) / 60,
		abs(zoneminutes) % 60
		);
	return Set(buf);
}

time_t TzWrapper::iso_mktime(const char *timestamp)
{
	bool utc, zone;
	struct tm t;
	int zoneminutes;
	if( !iso_to_tm(timestamp, &t, utc, &zone, &zoneminutes) )
		return (time_t)-1;

	TzWrapper tzw;
	if( utc ) {
		tzw.SetUTC();
	}
	else if( zone ) {
		tzw.SetOffset(zoneminutes);
	}
	return tzw.mktime(&t);
}

}} // namespace Barry::Sync


#ifdef TZ_TEST_MODE
#include <iostream>
using namespace std;
using namespace Barry::Sync;
int main()
{
	time_t now = time(NULL);

	cout << "TZ:             " << TzWrapper().ctime(&now);
	cout << "UTC:            " << TzWrapper("").ctime(&now);
	cout << "UTC:            " << TzWrapper().SetUTC().ctime(&now);
	cout << "SysLocaltime:   " << TzWrapper().SetUTC().SetSysLocal().ctime(&now);
	cout << "TZ:             " << TzWrapper().SetUTC().SetDefault().ctime(&now);
	cout << "Canada/Eastern: " << TzWrapper("Canada/Eastern").ctime(&now);
	cout << "Canada/Pacific: " << TzWrapper("Canada/Pacific").ctime(&now);

	{
		TzWrapper tzw("UTC");
		cout << "UTC:            " << ctime(&now);
	}

	cout << "TZ:             " << ctime(&now);

	// test iso_mktime()... the test values assume a Canada/Eastern TZ
	cout << "Using Canada/Eastern:" << endl;
	TzWrapper tzw("Canada/Eastern");
	const char *iso1 = "20100430T231500";
	const char *iso2 = "20100501T031500Z";
	time_t t1 = TzWrapper::iso_mktime(iso1);
	time_t t2 = TzWrapper::iso_mktime(iso2);
	cout << " " << iso1 << ": (" << t1 << ") " << ctime(&t1);
	cout        << iso2 << ": (" << t2 << ") " << ctime(&t2);
	if( t1 == t2 )
		cout << "t1 == t2: passed" << endl;
	else
		cout << "t1 != t2: ERROR" << endl;

	time_t t3 = TzWrapper::iso_mktime("2010-05-01T03:15:00.000Z");
	cout << ctime(&t3);
	if( t2 == t3 )
		cout << "t2 == t3: passed" << endl;
	else
		cout << "t2 != t3: ERROR" << endl;

	time_t t4 = TzWrapper::iso_mktime("2010-05-01T04:15:00.000+01:00");
	cout << ctime(&t4);
	if( t3 == t4 )
		cout << "t3 == t4: passed" << endl;
	else
		cout << "t3 != t4: ERROR" << endl;

	time_t t5 = TzWrapper::iso_mktime("2010-05-01T00:15:00.000-03:00");
	cout << ctime(&t5);
	if( t4 == t5 )
		cout << "t4 == t5: passed" << endl;
	else
		cout << "t4 != t5: ERROR: t4: " << t4 << " t5: " << t5  << endl;

	if( TzWrapper::iso_mktime("20100430") != (time_t)-1 )
		cout << "Date check: passed" << endl;
	else
		cout << "Date check: ERROR" << endl;

	cout << "t1: " << tm_to_iso(gmtime(&t1), true) << endl;

	bool utc, zone;
	int zoneminutes;
	struct tm zonetest;
	if( !iso_to_tm("2010-05-01T04:15:00.000-", &zonetest, utc, &zone, &zoneminutes) )
		cout << "iso_to_tm failed wrongly: ERROR" << endl;
	if( zone )
		cout << "zone true?: ERROR" << endl;
	else
		cout << "zone fail check: passed" << endl;

	if( !iso_to_tm("2010-05-01T04:15:00.000-010", &zonetest, utc, &zone, &zoneminutes) )
		cout << "iso_to_tm failed wrongly: ERROR" << endl;
	if( zone )
		cout << "zone true?: ERROR" << endl;
	else
		cout << "zone fail check2: passed" << endl;
}
#endif

