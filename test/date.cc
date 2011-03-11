///
/// \file	date.cc
///		Tests for the Date class
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

#include <barry/record.h>
#include "libtest.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string.h>
using namespace std;
using namespace Barry;

bool TestDate()
{
	struct tm t;
	memset(&t, 0, sizeof(t));
	t.tm_year = 111;
	t.tm_mon = 1;
	t.tm_mday = 28;

	Date d(&t);
	TEST( d.ToYYYYMMDD() == "20110228", "ToYYYYMMDD() failed");
	TEST( d.ToBBString() == "28/02/2011", "ToBBString() failed");

	ostringstream oss;
	oss << hex << d;
	TEST( oss.str() == "2011/02/28", "Stream output failed: " << oss.str());

	Date d2;
	d2.FromTm(&t);
	TEST( d2.ToYYYYMMDD() == "20110228", "FromTm() failed");

	struct tm myt;
	d.ToTm(&myt);
	d2.FromTm(&myt);
	TEST( d2.ToYYYYMMDD() == "20110228", "ToTm() failed");

	d2.FromBBString(d.ToBBString());
	TEST( d2.ToYYYYMMDD() == "20110228", "FromBBString() failed");

	d2.FromYYYYMMDD(d.ToYYYYMMDD());
	TEST( d2.ToYYYYMMDD() == "20110228", "FromYYYMMDD() failed");

	return true;
}

NewTest testdate("Date class", &TestDate);

