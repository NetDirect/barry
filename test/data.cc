///
/// \file	data.cc
///		Tests for the Data classes
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

#include <barry/data.h>
#include "libtest.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <stdexcept>
using namespace std;
using namespace Barry;

static bool Equal(const Data &d1, const Data &d2)
{
	return d1.GetSize() == d2.GetSize() &&
		memcmp(d1.GetData(), d2.GetData(), d1.GetSize()) == 0;
}

bool TestData()
{
//	typedef std::vector<Data> DataVec;
//	DataVec array;
//	if( !LoadDataArray("data/parsed.log", array) ) {
//		cout << "Can't load file" << endl;
//		return 1;
//	}

//	DataVec::iterator i = array.begin();
//	Data::PrintAscii(false);
//	for( ; i != array.end(); i++ ) {
//		cout << "Endpoint: " << i->GetEndpoint() << endl;
//		cout << *i;
//		cout << "\n\n";
//	}

	Data d;
	TEST( d.GetBufSize() == 0x4000, "Unexpected default buffer size");

	const char *str = "hello world";
	Data ed(str, strlen(str));
	TEST( ed.GetSize() == strlen(str),
		"Incorrect GetSize() on external data");
	TEST( ed.GetData() == (unsigned char*) str,
		"GetData() external data pointer not the same");

	bool caught = false;
	std::string msg;
	try { ed.ReleaseBuffer(1); }
	catch( std::logic_error &e ) {
		caught = true;
		msg = e.what();
	}
	TEST( caught == true,
		"ReleaseBuffer() didn't catch GetBuffer() order: " << msg);

	TEST( ed.GetBuffer() != (unsigned char*) str,
		"Data::GetBuffer() did not copy on write correctly");

	d = ed;
	TEST( Equal(d, ed), "Real copy didn't work");

	caught = false;
	try { ed.ReleaseBuffer(1300); }
	catch( std::logic_error & ) {
		caught = true;
	}
	TEST( caught == true, "Data::ReleaseBuffer() did not catch overflow");

	ed.QuickZap();
	TEST( ed.GetSize() == 0 && ed.GetData(), "QuickZap did not work");

	const unsigned char *old = ed.GetBuffer();
	Data ed2(str, strlen(str));
	ed = ed2;
	TEST( ed.GetData() == (unsigned char* )str,
		"operator=() did not recognize easy external data copy");
	TEST( ed.GetBuffer() == old,
		"GetBuffer() did not recover existing memBlock");
	TEST( Equal(ed, ed2), "Data not equal!\n" << ed << endl << ed2);

	const char *str2 = " and goodbye";
	const char *str3 = "hello world and goodbye";
	ed2.Append(str2, strlen(str2));
	Data ed3(str3, strlen(str3));
	TEST( Equal(ed2, ed3), "Append failed");

	old = ed.GetData();
	size_t old_size = ed.GetSize();
	ed.Prepend("pre", 3);
	TEST( (ed.GetData() + 3) == old,
		"Prepend buffer failed: "
			<< (void*)old << ":\n" << Data(old, old_size)
			<< (void*)ed.GetData() << ":\n" << ed);
	ed.Prechop(3);
	TEST( ed.GetData() == old, "Prechop failed");

	cout << "Examples of Diff() output" << endl;
	Data one, two;
	one.GetBuffer()[0] = 0x01;
	one.ReleaseBuffer(1);
	two.GetBuffer()[0] = 0x02;
	two.ReleaseBuffer(2);

	cout << Diff(one, two) << endl;
	cout << Diff(two, one) << endl;

	two.GetBuffer();
	two.ReleaseBuffer(32);
	cout << Diff(one, two) << endl;

	return true;
}

NewTest testdata("Data classes", &TestData);

