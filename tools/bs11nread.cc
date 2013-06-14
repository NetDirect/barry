///
/// \file	bs11nread.cc
///		Reads an boost serialization file and dumps to stdout.
///

/*
    Copyright (C) 2008-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

// Boost is required, but since the real Boost serialization code is in
// util.cc, we don't need it here.
#undef __BARRY_BOOST_MODE__

#include <barry/barry.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include "i18n.h"

#include "barrygetopt.h"
#include "util.h"
#include "boostwrap.h"

using namespace std;
using namespace Barry;

void Usage()
{
   int logical, major, minor;
   const char *Version = Barry::Version(logical, major, minor);

   cerr << string_vprintf(
   _("bs11nread - Reads a boost serialization file (from btool)\n"
   "            and dumps data to stdout\n"
   "        Copyright 2008-2013, Net Direct Inc. (http://www.netdirect.ca/)\n"
   "        Using: %s\n"
   "\n"
   "   -f file   Filename to save or load handheld data to/from\n"
   "   -h        This help\n"
   "   -S        Show list of supported database parsers\n"),
	Version)
   << endl;
}

template <class Record>
bool Dump(const std::string &dbName, const std::string &filename)
{
	if( dbName != Record::GetDBName() )
		return false;

	std::vector<Record> records;
	std::string junk, errmsg;

	if( !LoadBoostFile(filename, records, junk, errmsg) ) {
		cerr << errmsg << endl;
		return false;
	}

	cout << records.size()
	     << _(" records loaded") << endl;
	sort(records.begin(), records.end());

	typename std::vector<Record>::const_iterator
		beg = records.begin(), end = records.end();
	for( ; beg != end; beg++ ) {
		cout << (*beg) << endl;
	}

	return true;
}

void DumpDB(const string &filename)
{
	// filename is available, attempt to load
	ifstream ifs(filename.c_str());
	std::string dbName;
	getline(ifs, dbName);
	ifs.close();

	// check for recognized database names
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) Dump<tname>(dbName, filename) ||
	ALL_KNOWN_PARSER_TYPES
		cerr << _("Unknown database name: ") << dbName << endl;
}

int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	try {
		string filename;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "f:hS");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'f':	// filename
				filename = optarg;
				break;

			case 'S':	// show supported databases
				ShowParsers(false, false);
				return 0;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		// Initialize the barry library.  Must be called before
		// anything else.
		Barry::Init();

		if( !filename.size() ) {
			cerr << _("Filename must be specified") << endl;
			return 1;
		}

		DumpDB(filename);

	}
	catch( Usb::Error &ue) {
		std::cerr << _("Usb::Error caught: ") << ue.what() << endl;
		return 1;
	}
	catch( Barry::Error &se ) {
		std::cerr << _("Barry::Error caught: ") << se.what() << endl;
		return 1;
	}
	catch( std::exception &e ) {
		std::cerr << _("std::exception caught: ") << e.what() << endl;
		return 1;
	}

	return 0;
}

