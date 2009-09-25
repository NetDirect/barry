///
/// \file	main.cc
///		Program entry point for the desktop gui
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
#include "os22.h"
#include "os40.h"

using namespace std;

void Test22()
{
	OpenSync22 os22;
	cout << os22.osync_get_version() << endl;
}

void Test40()
{
	OpenSync40 os40;
	cout << os40.GetVersion() << endl;

	string_list_type plugins;
	os40.GetPluginNames(plugins);
	cout << plugins << endl;
}

int main()
{
	try {

		Test22();
		Test40();

		// test both at once
		OpenSync22 os22;
		cout << os22.osync_get_version() << endl;

		OpenSync40 os40;
		cout << os40.GetVersion() << endl;

		string_list_type plugins;
		os40.GetPluginNames(plugins);
		cout << plugins << endl;

	} catch(std::exception &e ) {
		cout << e.what() << endl;
		return 1;
	}

	return 0;
}

