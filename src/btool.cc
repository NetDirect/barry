///
/// \file	btool.cc
///		Barry library tester
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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



#include "common.h"
#include "probe.h"
#include "usbwrap.h"
#include "error.h"
#include "controller.h"
#include "parser.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <getopt.h>

using namespace std;
using namespace Barry;

void Usage()
{
   cerr
   << "bbtool - Command line USB Blackberry Test Tool\n"
   << "         Copyright 2005, Net Direct Inc. (http://www.netdirect.ca/)\n\n"
   << "   -c dn     Convert address book database to LDIF format, using the\n"
   << "             specified baseDN\n"
   << "   -d db     Load database 'db' and dump to screen\n"
   << "             Can be used multiple times to fetch more than one DB\n"
   << "   -h        This help\n"
   << "   -l        List devices\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device plugged in, this flag is optional\n"
   << "   -t        Show database database table\n"
   << endl;
}

class Contact2Ldif
{
	std::string m_baseDN;
public:
	Contact2Ldif(const std::string &baseDN) : m_baseDN(baseDN) {}
	void operator()(const Contact &rec)
	{
		rec.DumpLdif(cout, m_baseDN);
	}
};

template <class Record>
struct Store
{
	int count;
	Store() : count(0) {}
	~Store()
	{
		cout << "Store counted " << count << " records." << endl;
	}

	// just dump to screen for now
	void operator()(const Record &rec)
	{
		count++;
		std::cout << rec << std::endl;
	}
};

auto_ptr<Parser> GetParser(const string &name)
{
	// check for recognized database names
	if( name == "Address Book" ) {
		return auto_ptr<Parser>(
			new RecordParser<Contact, Store<Contact> > (
				new Store<Contact>));
	}
	else if( name == "Messages" ) {
		return auto_ptr<Parser>(
			new RecordParser<Message, Store<Message> > (
				new Store<Message>));
	}
	else {
		// unknown database, use null parser
		return auto_ptr<Parser>( new Parser );
	}
}

int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool	list_only = false,
			show_dbdb = false,
			ldif_contacts = false;
		string ldifBaseDN;
		vector<string> dbNames;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "c:d:hlp:t");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'c':	// contacts to ldap ldif
				ldif_contacts = true;
				ldifBaseDN = optarg;
				break;

			case 'd':	// show dbname
				dbNames.push_back(string(optarg));
				break;

			case 'l':	// list only
				list_only = true;
				break;

			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 't':	// display database database
				show_dbdb = true;
				break;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		Init();

		Probe probe;
		int activeDevice = -1;
		cout << "Blackberry devices found:" << endl;
		for( int i = 0; i < probe.GetCount(); i++ ) {
			cout << probe.Get(i) << endl;
			if( probe.Get(i).m_pin == pin )
				activeDevice = i;
		}

		if( list_only )
			return 0;	// done

		if( activeDevice == -1 ) {
			if( pin == 0 ) {
				// can we default to single device?
				if( probe.GetCount() == 1 )
					activeDevice = 0;
				else {
					cerr << "No device selected" << endl;
					return 1;
				}
			}
			else {
				cerr << "PIN " << setbase(16) << pin
					<< " not found" << endl;
				return 1;
			}
		}

		// create our controller object
		Controller con(probe.Get(activeDevice));

		// execute each mode that was turned on

		if( show_dbdb ) {
			// open desktop mode socket
			con.OpenMode(Controller::Desktop);
			cout << con.GetDBDB() << endl;
		}

		if( ldif_contacts ) {
			con.OpenMode(Controller::Desktop);
			Contact2Ldif storage(ldifBaseDN);
			RecordParser<Contact, Contact2Ldif> parser(storage);
			con.LoadDatabase(con.GetDBID("Address Book"), parser);
		}

		if( dbNames.size() ) {
			vector<string>::iterator b = dbNames.begin();

			for( ; b != dbNames.end(); b++ ) {
				con.OpenMode(Controller::Desktop);
				unsigned int id = con.GetDBID(*b);
				auto_ptr<Parser> parse = GetParser(*b);
				con.LoadDatabase(id, *parse.get());
			}
		}

	}
	catch( Barry::SBError &se ) {
		std::cerr << "SBError caught: " << se.what() << endl;
	}
	catch( Usb::UsbError &ue) {
		std::cerr << "UsbError caught: " << ue.what() << endl;
	}
	catch( std::runtime_error &re ) {
		std::cerr << "std::runtime_error caught: " << re.what() << endl;
		return 1;
	}

	return 0;
}

