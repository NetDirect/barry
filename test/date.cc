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

#include <barry/barry.h>
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
	if( d.ToYYYYMMDD() != "20110228" ) {
		cout << "ToYYYYMMDD() failed" << endl;
		return false;
	}

	if( d.ToBBString() != "28/02/2011" ) {
		cout << "ToBBString() failed" << endl;
		return false;
	}

	ostringstream oss;
	oss << hex << d;
	if( oss.str() != "2011/02/28" ) {
		cout << "Stream output failed: " << oss.str() << endl;
		return false;
	}

	Date d2;
	d2.FromTm(&t);
	if( d2.ToYYYYMMDD() != "20110228" ) {
		cout << "FromTm() failed" << endl;
		return false;
	}

	struct tm myt;
	d.ToTm(&myt);
	d2.FromTm(&myt);
	if( d2.ToYYYYMMDD() != "20110228" ) {
		cout << "ToTm() failed" << endl;
		return false;
	}

	d2.FromBBString(d.ToBBString());
	if( d2.ToYYYYMMDD() != "20110228" ) {
		cout << "FromBBString() failed" << endl;
		return false;
	}

	d2.FromYYYYMMDD(d.ToYYYYMMDD());
	if( d2.ToYYYYMMDD() != "20110228" ) {
		cout << "FromYYYMMDD() failed" << endl;
		return false;
	}

	return true;
}

NewTest testdate("Date class", &TestDate);

