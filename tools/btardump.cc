///
/// \file	tardump.cc
///		Utility to dump tarball	backup records to stdout.
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
#ifdef __BARRY_SYNC_MODE__
#include <barry/barrysync.h>
#include "mimedump.h"
#endif
#include <barry/barrybackup.h>
#include <iostream>
#include <iomanip>
#include <getopt.h>

using namespace std;
using namespace Barry;

void Usage()
{
   int logical, major, minor;
   const char *Version = Barry::Version(logical, major, minor);

   cerr
   << "btardump - Command line parser for Barry backup files\n"
   << "           Copyright 2010-2011, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "           Using: " << Version << "\n"
   << "\n"
   << "   -d db     Name of database to dump.  Can be used multiple times\n"
   << "             to parse multiple databases at once.  If not specified\n"
   << "             at all, all available databases from the backup are\n"
   << "             dumped.\n"
   << "   -h        This help\n"
   << "   -i cs     International charset for string conversions\n"
   << "             Valid values here are available with 'iconv --list'\n"
#ifdef __BARRY_SYNC_MODE__
   << "   -V        Dump records using MIME vformats where possible\n"
#endif
   << "\n"
   << "   [files...] Backup file(s), created by btool or the backup GUI.\n"
   << endl;
}

class MyAllRecordDumpStore : public AllRecordStore
{
	bool m_vformat_mode;
	std::ostream &m_os;

public:
	explicit MyAllRecordDumpStore(std::ostream &os, bool vformat_mode=false)
		: m_vformat_mode(vformat_mode)
		, m_os(os)
	{
	}

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	void operator() (const Barry::tname &r) \
	{ \
		if( m_vformat_mode ) \
			MimeDump<tname>::Dump(m_os, r); \
		else \
			m_os << r << std::endl; \
	}

	ALL_KNOWN_PARSER_TYPES
};

int main(int argc, char *argv[])
{
	try {
		bool vformat_mode = false;

		vector<string> db_names;
		vector<string> backup_files;
		string iconvCharset;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "d:hi:V");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'd':	// show dbname
				db_names.push_back(string(optarg));
				break;

			case 'V':	// vformat MIME mode
#ifdef __BARRY_SYNC_MODE__
				vformat_mode = true;
#else
				cerr << "-V option not supported - no Sync "
					"library support available\n";
				return 1;
#endif
				break;

			case 'i':	// international charset (iconv)
				iconvCharset = optarg;
				break;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		// grab all backup filenames
		while( optind < argc ) {
			backup_files.push_back(string(argv[optind++]));
		}

		if( backup_files.size() == 0 ) {
			Usage();
			return 0;
		}



		Barry::Init();

		// Create an IConverter object if needed
		auto_ptr<IConverter> ic;
		if( iconvCharset.size() ) {
			ic.reset( new IConverter(iconvCharset.c_str(), true) );
		}

		// create the parser, and use stdout dump objects for output
		AllRecordParser parser(cout,
			new HexDumpParser(cout),
			new MyAllRecordDumpStore(cout, vformat_mode));

		for( size_t i = 0; i < backup_files.size(); i++ ) {

			cout << "Reading file: " << backup_files[i] << endl;

			Restore builder(backup_files[i]);

			// add desired database names
			for( size_t j = 0; j < db_names.size(); j++ ) {
				builder.AddDB(db_names[i]);
			}

			// create the pipe to connect builder to parser and
			// move the data
			Pipe pipe(builder);
			pipe.PumpFile(parser, ic.get());
		}

	}
	catch( exception &e ) {
		cerr << e.what() << endl;
		return 1;
	}

	return 0;
}

