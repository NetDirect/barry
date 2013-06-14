///
/// \file	upldif.cc
///		LDIF contact uploader
///

/*
    Copyright (C) 2006-2013, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <string>
#include "i18n.h"

#include "barrygetopt.h"

using namespace std;
using namespace Barry;

void Usage()
{
   cerr <<
   _("upldif - Command line LDIF uploader\n"
   "         Copyright 2006-2013, Net Direct Inc. (http://www.netdirect.ca/)\n\n"
   "   -p pin    PIN of device to talk with\n"
   "             If only one device plugged in, this flag is optional\n"
   "   -P pass   Simplistic method to specify device password\n"
   "   -u        Do the upload.  If not specified, only dumps parsed\n"
   "             LDIF data to stdout.\n"
   "   -v        Dump protocol data during operation\n"
   "   -h        This help output\n")
   << endl;
}

template <class Record>
struct Store
{
	std::vector<Record> records;
	mutable typename std::vector<Record>::const_iterator rec_it;
	int count;

	Barry::ContactLdif ldif;

	// Store constructor -- reads LDIF records from the given
	// stream object and stores them in memory.
	Store(std::istream &is)
		: count(0),
		ldif("")
	{
		Record rec;
		while( is ) {
			if( ldif.ReadLdif(is, rec) ) {
				count++;
				records.push_back(rec);
			}
		}

		rec_it = records.begin();
	}

	~Store()
	{
		cout << string_vprintf(_("Store counted %d records."), count) << endl;
	}

	// Retrieval operator -- called by Barry during the upload
	// process to get the next object
	bool operator()(Record &rec, Builder &builder) const
	{
		if( rec_it == records.end() )
			return false;
		rec = *rec_it;
		rec_it++;
		return true;
	}

	// For easy data display and debugging.
	void Dump(std::ostream &os) const
	{
		typename std::vector<Record>::const_iterator b = records.begin();
		for( ; b != records.end(); ++b ) {
			os << *b << endl;
		}
	}
};

template <class Record>
std::ostream& operator<< (std::ostream &os, const Store<Record> &store)
{
	store.Dump(os);
	return os;
}

int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool	data_dump = false,
			do_upload = false;
		string password;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "hp:P:uv");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 'P':	// Device password
				password = optarg;
				break;

			case 'u':	// do upload
				do_upload = true;
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

		// Read all contacts from stdin
		Store<Contact> contactStore(cin);

		// Only dump to stdout if not uploading to device
		if( !do_upload ) {
			cout << contactStore << endl;
			return 0;
		}

		// Initialize the barry library.  Must be called before
		// anything else.
		Barry::Init(data_dump);

		// Probe the USB bus for Blackberry devices
		// If user has specified a PIN, search for it
		Barry::Probe probe;
		int activeDevice = probe.FindActive(pin);
		if( activeDevice == -1 ) {
			cerr << _("Device not found, or not specified") << endl;
			return 1;
		}

		// Create our controller object
		Barry::Controller con(probe.Get(activeDevice));

		// make sure we're in desktop mode
		Barry::Mode::Desktop desktop(con);
		desktop.Open(password.c_str());

		// upload all records to device
		desktop.SaveDatabaseByType<Barry::Contact>(contactStore);

	}
	catch( Usb::Error &ue) {
		std::cerr << _("Usb::Error caught: ") << ue.what() << endl;
	}
	catch( Barry::Error &se ) {
		std::cerr << _("Barry::Error caught: ") << se.what() << endl;
	}
	catch( std::exception &e ) {
		std::cerr << _("std::exception caught: ") << e.what() << endl;
		return 1;
	}

	return 0;
}

