///
/// \file	time.cc
///		Conversion between time_t and cenmin_t and back.
///		time_t is the POSIX time, seconds from Jan 1, 1970
///		min1900_t is the minutes from Jan 1, 1900
///

/*
    Copyright (C) 2005-2006, Net Direct Inc. (http://www.netdirect.ca/)

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
}

#endif

