///
/// \file	tzwrapper.cc
///		Timezone adjustment class, wrapping the TZ environment
///		variable to make struct tm -> time_t conversions easier.
///

/*
    Copyright (C) 2010, Chris Frey <cdfrey@foursquare.net>, To God be the glory
    Released to the public domain.
    Included in Barry and Barrified the namespace July 2010
*/

#include "tzwrapper.h"
#include <string.h>
#include <stdio.h>

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
			bool &utc)
{
	memset(result, 0, sizeof(struct tm));
	char zflag = 0;

	int found = sscanf(timestamp, "%04d%02d%02dT%02d%02d%02d%c",
		&(result->tm_year), &(result->tm_mon), &(result->tm_mday),
		&(result->tm_hour), &(result->tm_min), &(result->tm_sec),
		&zflag);

	result->tm_year -= 1900;
	result->tm_mon -= 1;
	result->tm_isdst = -1;

	utc = (found == 7 && zflag == 'Z');

	return (found >= 6) ? result : 0;
}

time_t TzWrapper::iso_mktime(const char *timestamp)
{
	bool utc;
	struct tm t;
	if( !iso_to_tm(timestamp, &t, utc) )
		return (time_t)-1;
	TzWrapper tzw;
	if( utc )
		tzw.SetUTC();
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

	if( TzWrapper::iso_mktime("20100430") == (time_t)-1 )
		cout << "Fail check: passed" << endl;
	else
		cout << "Fail check: ERROR" << endl;
}
#endif

