///
/// \file	dbdump.cc
///		Example code using the Barry library to pull all
///		contacts out of the device and dump to stdout.
///

/*
    Copyright (C) 2010-2011, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <iostream>

using namespace std;
using namespace Barry;

struct Callback
{
	// storage operator
	void operator()(Contact &rec)
	{
		// do something with your record here
		cout << rec << endl;
	}
};

int main(int argc, char *argv[])
{
	try {

		Barry::Init();
		Probe probe;
		int i = probe.FindActive();
		if( i == -1 ) {
			cout << "No PIN specified" << endl;
			return 1;
		}

		Controller con(probe.Get(i));
		Mode::Desktop desktop(con);
		desktop.Open();	// specify password here if needed

		Callback storage;
		desktop.LoadDatabaseByType<Contact>(storage);

	}
	catch( exception &e ) {
		cerr << e.what() << endl;
		return 1;
	}

	return 0;
}

