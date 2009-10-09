///
/// \file	ostest.cc
///		Test application for the OpenSync API
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <iostream>
#include <stdexcept>
#include <memory>
#include <barry/barry.h>
#include "os22.h"
#include "os40.h"

using namespace std;
using namespace OpenSync;

void Test(API &os)
{
	cout << "=======================================================\n";
	cout << " Begin test run\n";
	cout << "=======================================================\n";

	cout << os.GetVersion() << endl;

	string_list_type slist;
	os.GetPluginNames(slist);
	cout << "Plugins:\n" << slist << endl;

	os.GetGroupNames(slist);
	cout << "Groups:\n" << slist << endl;

	for( string_list_type::iterator b = slist.begin(); b != slist.end(); ++ b) {
		member_list_type mlist;
		os.GetMembers(*b, mlist);
		cout << "Members for group: " << *b << endl;
		cout << "---------------------------------------\n";
		cout << mlist << endl;
	}
}

int main()
{
	Barry::Init(true);

	try {
		APISet set;
		set.OpenAvailable();

		if( set.os40() ) {
			Test(*set.os40());
		}

		if( set.os22() ) {
			Test(*set.os22());
		}
	}
	catch( std::exception &e ) {
		cout << e.what() << endl;
	}

	return 0;
}

