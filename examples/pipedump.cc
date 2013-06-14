///
/// \file	dumpall.cc
///		Example code using the Barry library to dump all
///		device databases to stdout, using all known parsers,
///		and hex dumps for the rest.
///

/*
    Copyright (C) 2011-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

using namespace std;
using namespace Barry;

template <class RecordT>
class Store
{
public:
	void operator() (const RecordT &rec)
	{
		cout << RecordT::GetDBName() << ": "
			<< hex << rec.GetUniqueId() << endl;
	}
};

class UnknownParser : public Parser
{
public:
	void ParseRecord(const DBData &data, const IConverter *ic)
	{
		cout << "Unknown record of "
			<< dec << data.GetData().GetSize()
			<< " bytes from: "
			<< data.GetDBName()
			<< endl;
	}
};

int main(int argc, char *argv[])
{
	try {

		Barry::Init();
		Probe probe;
		int i = probe.FindActive();
		if( i == -1 ) {
			cout << "No device available" << endl;
			return 1;
		}

		// open Desktop mode
		Controller con(probe.Get(i));
		Mode::Desktop desktop(con);
		desktop.Open();	// specify password here if needed

		// create builder object to extract records from the
		// device, and add all database names to the builder
		DeviceBuilder builder(desktop);
		const DatabaseDatabase &dbdb = desktop.GetDBDB();
		DatabaseDatabase::DatabaseArrayType::const_iterator
			b = dbdb.Databases.begin(), e = dbdb.Databases.end();
		for( ; b != e; ++b ) {
			builder.Add( b->Name );
		}

		// create the parser, and use stdout dump objects for output
		AllRecordParser parser(cout,
			new HexDumpParser(cout),
			new AllRecordDumpStore(cout));

		// create the pipe to connect builder to parser and
		// move the data
		Pipe pipe(builder);
		pipe.PumpFile(parser);

		cout << "\n\n\nStarting again....................." << endl;

		// run it again, but this time with custom set of
		// record parsers and default parser

		MultiRecordParser mrp( new UnknownParser );

		// add a few known record types... first, the manual way
		mrp.Add( Contact::GetDBName(),
			new RecordParser<Contact, Store<Contact> >(
				new Store<Contact>));
		// and with the template member (does the same thing)
		mrp.Add<Calendar>( new Store<Calendar> );
		mrp.Add<Sms>( new Store<Sms> );
		mrp.Add<TimeZone>( new Store<TimeZone> );

		builder.Restart();
		pipe.PumpFile(mrp);

		// and one more time, with a tee
		cout << "\n\n\nStarting again with a tee............" << endl;

		TeeParser tee;
		tee.Add( mrp );
		tee.Add( parser );

		builder.Restart();
		pipe.PumpFile(tee);

	}
	catch( exception &e ) {
		cerr << e.what() << endl;
		return 1;
	}

	return 0;
}

