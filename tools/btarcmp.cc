///
/// \file	btarcmp.cc
///		Compare / diff tool to analyze Barry backup tarballs
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <barry/barrybackup.h>

//#include "brecsum.h"

#include <iostream>
#include <iomanip>

#include "barrygetopt.h"

using namespace std;
using namespace Barry;

void Usage()
{
   int logical, major, minor;
   const char *Version = Barry::Version(logical, major, minor);

   cerr
   << "btarcmp - Compare Barry backup tarballs\n"
   << "      Copyright 2012, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "      Using: " << Version << "\n"
   << "\n"
   << " Usage:  btarcmp [options...] tarball1 tarball2\n"
   << "\n"
   << "   -h        This help\n"
   << "   -I cs     International charset for string conversions\n"
   << "             Valid values here are available with 'iconv --list'\n"
   << "   -S        Show list of supported database parsers\n"
   << "   -v        Show verbose diff output\n"
   << "\n"
   << endl;
}

//////////////////////////////////////////////////////////////////////////////
// Main application class

class App
{
private:

public:
	static void ShowParsers();

	// returns true if any of the items in Outputs needs a probe
	int main(int argc, char *argv[]);
};

void App::ShowParsers()
{
	cout << "Supported Database parsers:\n"

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	<< "   " << tname::GetDBName() << "\n"

	ALL_KNOWN_PARSER_TYPES

	<< endl;
}

int App::main(int argc, char *argv[])
{
	bool verbose = false;
	string iconvCharset;

	// process command line options
	for(;;) {
		int cmd = getopt(argc, argv, "hi:o:Sv");
		if( cmd == -1 )
			break;

		switch( cmd )
		{
		case 'S':	// show parsers and builders
			ShowParsers();
			return 0;

		case 'I':	// international charset (iconv)
			iconvCharset = optarg;
			break;

		case 'v':	// verbose
			verbose = true;
			break;

		case 'h':	// help
		default:
			Usage();
			return 0;
		}
	}

	if( (optind + 2) >= argc ) {
		Usage();
		return 0;
	}

	// Initialize the Barry library
	Barry::Init(verbose);

	return 0;
}

int main(int argc, char *argv[])
{
	try {
		App app;
		return app.main(argc, argv);
	}
	catch( std::exception &e ) {
		cerr << "Exception: " << e.what() << endl;
		return 1;
	}
}

