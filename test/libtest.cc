///
/// \file	libtest.cc
///		Routines for testing the Barry libraries
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "libtest.h"
#include <vector>
#include <iostream>
#include <iomanip>
using namespace std;

typedef std::pair<const char*, testfunc> TestPair;
typedef std::vector<TestPair> TestList;

TestList& GetTests()
{
	static TestList tests;
	return tests;
}

void AddTest(const char *name, testfunc test)
{
	GetTests().push_back(TestPair(name, test));
}

int main()
{
	int failures = 0;

	TestList::iterator b = GetTests().begin(), e = GetTests().end();
	for( ; b != e; ++b ) {
		cout << "Testing: " << b->first << "... " << flush;
		if( !(*b->second)() ) {
			failures++;
			cout << "FAILED" << endl;
		}
		else {
			cout << "passed" << endl;
		}
	}

	if( failures ) {
		cout << dec << failures << " tests failed" << endl;
	}
	else {
		cout << "All tests passed" << endl;
	}

	return failures ? 1 : 0;
}

