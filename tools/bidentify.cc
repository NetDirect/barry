///
/// \file	bidentify.cc
///		Tool for probing identifying Blackberry devices
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <iomanip>
#include <getopt.h>
#include "i18n.h"

using namespace std;
using namespace Barry;

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "bidentify - USB Blackberry Identifier Tool\n"
   << "            Copyright 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "            Using: " << Version << "\n"
   << "\n"
   << "   -B bus    Specify which USB bus to search on\n"
   << "   -N dev    Specify which system device, using system specific string\n"
   << "\n"
   << "   -h        This help\n"
   << "   -v        Dump protocol data during operation\n"
   << endl;
}

int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		bool data_dump = false;
		string busname;
		string devname;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "B:hN:v");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'B':	// busname
				busname = optarg;
				break;

			case 'N':	// Devname
				devname = optarg;
				break;

			case 'v':	// data dump on
				data_dump = true;
				break;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		Barry::Init(data_dump);
		Barry::Probe probe(busname.c_str(), devname.c_str());

		// show any errors during probe first
		if( probe.GetFailCount() ) {
			cerr << "Blackberry device errors with errors during probe:" << endl;
			for( int i = 0; i < probe.GetFailCount(); i++ ) {
				cerr << probe.GetFailMsg(i) << endl;
			}
		}

		// show all successfully found devices
		for( int i = 0; i < probe.GetCount(); i++ ) {
			const ProbeResult &pr = probe.Get(i);
			cout 	<< pr.m_pin.Str() << ", "
				<< pr.m_description << endl;
		}

		return probe.GetFailCount();

	}
	catch( std::exception &e ) {
		cerr << "exception caught: " << e.what() << endl;
		return 1;
	}
}

