///
/// \file	balxparse.cc
///
///

/*
    Copyright (C) 2009-2010, Nicolas VIVIEN
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
#include <barry/barryalx.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <getopt.h>
#include "i18n.h"

using namespace std;
using namespace Barry;

struct Languages {
	const char *code;
	const char *alxid;
	const char *description;
};


const static struct Languages langs[] = {
	{ "en", OS_LANG_ENGLISH, "English" },
	{ "ar", OS_LANG_ARABIC, "Arabic" },
	{ "ca", OS_LANG_CATALAN, "Catalan" },
	{ "cs", OS_LANG_CZECH, "Czech" },
	{ "de", OS_LANG_GERMAN, "German" },
	{ "sp", OS_LANG_SPANISH, "Spanish" },
	{ "fr", OS_LANG_FRENCH, "French" },
	{ "he", OS_LANG_HEBREW, "Hebrew" },
	{ "hu", OS_LANG_HUNGARIAN, "Hungarian" },
	{ "it", OS_LANG_ITALIAN, "Italian" },
	{ "ja", OS_LANG_JAPANESE, "Japanese" },
	{ "ko", OS_LANG_KOREAN, "Korean" },
	{ NULL }
};

void Usage()
{
	int major, minor;
	const char *Version = Barry::Version(major, minor);

	cerr
	<< "balxparse - Command line ALX parser\n"
	<< "        Copyright 2009-2010, Nicolas VIVIEN.\n"
	<< "        Copyright 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)\n"
	<< "        Using: " << Version << "\n"
	<< "\n"
	<< "   -h        This help\n"
	<< "   -i lang   Internationalization language\n"
	<< "   -d path   OS path with all ALX files\n"
	<< "   -o file   OS ALX filename (Platform.alx)\n"
	<< "\n"
	<< "   <ALX file> ...\n"
	<< "     Parse one or several ALX files.\n"
	<< "\n"
	<< "   Language supported :\n"
	<< "\t";

	for (int i=0; langs[i].code!=NULL; i++) {
		string s = (string) langs[i].code + " : " + (string) langs[i].description;

		cerr << left << setfill(' ') << setw(18) << s;

		if (((i+1) % 4) == 0)
			cerr << endl << "\t";
	}
	
	cerr << endl;
}


int main(int argc, char *argv[], char *envp[])
{
	INIT_I18N(PACKAGE);

	try {

	string lang;
	string pathname;
	string filename;
	string osfilename;
	vector<string> filenames;

	// process command line options
	for(;;) {
		int cmd = getopt(argc, argv, "hi:d:o:");
		if( cmd == -1 )
			break;

		switch( cmd )
		{
		case 'd':	// ALX path
			pathname = optarg;
			break;

		case 'o':	// OS ALX filename (Platform.alx)
			osfilename = optarg;
			break;

		case 'i':	// Language
			lang = optarg;
			break;

		case 'h':	// help
		default:
			Usage();
			return 0;
		}
	}

	argc -= optind;
	argv += optind;

	// Put the remaining arguments into an array
	for (; argc > 0; argc --, argv ++) {
		filenames.push_back(string(argv[0]));
	}


	// Init ALX parser
	ALX::OSLoader os;

	os.addProperties("_vendorID", "");


	if (lang.length() > 0) {
		for (int i=0; langs[i].code!=NULL; i++) {
			string code = langs[i].code;

			if (code == lang)
				os.addProperties("langid", langs[i].alxid);
		}
	}

	if (osfilename.length() > 0)
		os.loadALXFile(osfilename, false);

	if (pathname.length() > 0)
		os.load(pathname);
	
	if (!filenames.empty()) {
		vector<string>::iterator i = filenames.begin(), end = filenames.end();
		for( ; i != end; ++i ) {
			os.loadALXFile((*i), true);
		}
	}
	
	cout << os << endl;


	} catch( std::exception &e ) {
		cout << e.what() << endl;
		return 1;
	}

	return 0;
}

