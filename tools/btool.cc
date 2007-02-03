///
/// \file	btool.cc
///		Barry library tester
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <string>
#include <getopt.h>


using namespace std;
using namespace Barry;

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "btool - Command line USB Blackberry Test Tool\n"
   << "        Copyright 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "        Using: " << Version << "\n"
   << "\n"
   << "   -c dn     Convert address book database to LDIF format, using the\n"
   << "             specified baseDN\n"
   << "   -d db     Load database 'db' FROM device and dump to screen\n"
   << "             Can be used multiple times to fetch more than one DB\n"
#ifdef __BARRY_BOOST_MODE__
   << "   -f file   Filename to save or load handheld data to/from\n"
#endif
   << "   -h        This help\n"
   << "   -l        List devices\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device plugged in, this flag is optional\n"
   << "   -s db     Save database 'db' TO device from data loaded from -f file\n"
   << "   -t        Show database database table\n"
   << "   -T db     Show record state table for given database\n"
   << "   -v        Dump protocol data during operation\n"
   << "   -X        Reset device\n"
   << "\n"
   << " -d Command modifiers:   (can be used multiple times for more than 1 record)\n"
   << "\n"
   << "   -r #      Record index number as seen in the -T state table.\n"
   << "             This overrides the default -d behaviour, and only\n"
   << "             downloads the one specified record, sending to stdout.\n"
   << "   -R #      Same as -r, but also clears the record's dirty flags.\n"
   << "   -D #      Record index number as seen in the -T state table,\n"
   << "             which indicates the record to delete.  Used with the -d\n"
   << "             command to specify the database.\n"
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
	std::vector<Record> records;
	mutable typename std::vector<Record>::const_iterator rec_it;
	std::string filename;
	bool load;
	int count;

	Store(const string &filename, bool load)
		: rec_it(records.end()),
		filename(filename),
		load(load),
		count(0)
	{
#ifdef __BARRY_BOOST_MODE__
		try {

			if( load && filename.size() ) {
				// filename is available, attempt to load
				cout << "Loading: " << filename << endl;
				ifstream ifs(filename.c_str());
				boost::archive::text_iarchive ia(ifs);
				ia >> records;
				cout << records.size()
				     << " records loaded from '"
				     << filename << "'" << endl;
				sort(records.begin(), records.end());
				rec_it = records.begin();

				// debugging aid
				typename std::vector<Record>::const_iterator beg = records.begin(), end = records.end();
				for( ; beg != end; beg++ ) {
					cout << (*beg) << endl;
				}
			}

		} catch( boost::archive::archive_exception &ae ) {
			cerr << "Archive exception in ~Store(): "
			     << ae.what() << endl;
		}
#endif
	}
	~Store()
	{
		cout << "Store counted " << dec << count << " records." << endl;
#ifdef __BARRY_BOOST_MODE__
		try {

			if( !load && filename.size() ) {
				// filename is available, attempt to save
				cout << "Saving: " << filename << endl;
				const std::vector<Record> &r = records;
				ofstream ofs(filename.c_str());
				boost::archive::text_oarchive oa(ofs);
				oa << r;
				cout << dec << r.size() << " records saved to '"
					<< filename << "'" << endl;
			}

		} catch( boost::archive::archive_exception &ae ) {
			cerr << "Archive exception in ~Store(): "
			     << ae.what() << endl;
		}
#endif
	}

	// storage operator
	void operator()(const Record &rec)
	{
		count++;
		std::cout << rec << std::endl;
		records.push_back(rec);
	}

	// retrieval operator
	bool operator()(Record &rec, unsigned int databaseId) const
	{
		if( rec_it == records.end() )
			return false;
		rec = *rec_it;
		rec_it++;
		return true;
	}
};

class DataDumpParser : public Barry::Parser
{
	uint32_t m_id;

public:
	virtual void SetUniqueId(uint32_t id)
	{
		m_id = id;
	}

	virtual void ParseFields(const Barry::Data &data, size_t &offset)
	{
		std::cout << "Raw record dump for record: "
			<< std::hex << m_id << std::endl;
		std::cout << data << std::endl;
	}
};

auto_ptr<Parser> GetParser(const string &name, const string &filename)
{
	// check for recognized database names
	if( name == "Address Book" ) {
		return auto_ptr<Parser>(
			new RecordParser<Contact, Store<Contact> > (
				new Store<Contact>(filename, false)));
	}
	else if( name == "Messages" ) {
		return auto_ptr<Parser>(
			new RecordParser<Message, Store<Message> > (
				new Store<Message>(filename, false)));
	}
	else if( name == "Calendar" ) {
		return auto_ptr<Parser>(
			new RecordParser<Calendar, Store<Calendar> > (
				new Store<Calendar>(filename, false)));
	}
	else if( name == "Service Book" ) {
		return auto_ptr<Parser>(
			new RecordParser<ServiceBook, Store<ServiceBook> > (
				new Store<ServiceBook>(filename, false)));
	}
	else {
		// unknown database, use null parser
		return auto_ptr<Parser>( new DataDumpParser );
	}
}

auto_ptr<Builder> GetBuilder(const string &name, const string &filename)
{
	// check for recognized database names
	if( name == "Address Book" ) {
		return auto_ptr<Builder>(
			new RecordBuilder<Contact, Store<Contact> > (
				new Store<Contact>(filename, true)));
	}
/*
	else if( name == "Messages" ) {
		return auto_ptr<Parser>(
			new RecordParser<Message, Store<Message> > (
				new Store<Message>(filename, true)));
	}
	else if( name == "Calendar" ) {
		return auto_ptr<Parser>(
			new RecordParser<Calendar, Store<Calendar> > (
				new Store<Calendar>(filename, true)));
	}
	else if( name == "Service Book" ) {
		return auto_ptr<Parser>(
			new RecordParser<ServiceBook, Store<ServiceBook> > (
				new Store<ServiceBook>(filename, true)));
	}
*/
	else {
		throw std::runtime_error("No Builder available for database");
	}
}

struct StateTableCommand
{
	char flag;
	bool clear;
	unsigned int index;

	StateTableCommand(char f, bool c, unsigned int i)
		: flag(f), clear(c), index(i) {}
};

int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool	list_only = false,
			show_dbdb = false,
			ldif_contacts = false,
			data_dump = false,
			reset_device = false,
			record_state = false;
		string ldifBaseDN;
		string filename;
		vector<string> dbNames, saveDbNames;
		vector<StateTableCommand> stCommands;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "c:d:D:f:hlp:r:R:s:tT:vX");
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

			case 'D':	// delete record
				stCommands.push_back(
					StateTableCommand('D', false, atoi(optarg)));
				break;

			case 'f':	// filename
#ifdef __BARRY_BOOST_MODE__
				filename = optarg;
#else
				cerr << "-f option not supported - no Boost "
					"serialization support available\n";
				return 1;
#endif
				break;
			case 'l':	// list only
				list_only = true;
				break;

			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 'r':	// get specific record index
				stCommands.push_back(
					StateTableCommand('r', false, atoi(optarg)));
				break;

			case 'R':	// same as 'r', and clears dirty
				stCommands.push_back(
					StateTableCommand('r', true, atoi(optarg)));
				break;

			case 's':	// save dbname
				saveDbNames.push_back(string(optarg));
				break;

			case 't':	// display database database
				show_dbdb = true;
				break;

			case 'T':	// show RecordStateTable
				record_state = true;
				dbNames.push_back(string(optarg));
				break;

			case 'v':	// data dump on
				data_dump = true;
				break;

			case 'X':	// reset device
				reset_device = true;
				break;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		// Initialize the barry library.  Must be called before
		// anything else.
		Barry::Init(data_dump);

		// Probe the USB bus for Blackberry devices and display.
		// If user has specified a PIN, search for it in the
		// available device list here as well
		Barry::Probe probe;
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

		if( reset_device ) {
			Usb::Device dev(probe.Get(activeDevice).m_dev);
			dev.Reset();
			return 0;
		}

		// Create our controller object
		Barry::Controller con(probe.Get(activeDevice));

		//
		// execute each mode that was turned on
		//


		// Dump list of all databases to stdout
		if( show_dbdb ) {
			// open desktop mode socket
			con.OpenMode(Controller::Desktop);
			cout << con.GetDBDB() << endl;
		}

		// Dump list of contacts to an LDAP LDIF file
		// This uses the Controller convenience templates
		if( ldif_contacts ) {
			// make sure we're in desktop mode
			con.OpenMode(Controller::Desktop);

			// create a storage functor object that accepts
			// Barry::Contact objects as input
			Contact2Ldif storage(ldifBaseDN);

			// load all the Contact records into storage
			con.LoadDatabaseByType<Barry::Contact>(storage);
		}

		// Dump record state table to stdout
		if( record_state ) {
			if( dbNames.size() == 0 ) {
				cout << "No db names to process" << endl;
				return 1;
			}

			vector<string>::iterator b = dbNames.begin();
			for( ; b != dbNames.end(); b++ ) {
				con.OpenMode(Controller::Desktop);
				unsigned int id = con.GetDBID(*b);
				RecordStateTable state;
				con.GetRecordStateTable(id, state);
				cout << "Record state table for: " << *b << endl;
				cout << state;
			}
			return 0;
		}

		// Get Record mode overrides the default name mode
		if( stCommands.size() ) {
			if( dbNames.size() != 1 ) {
				cout << "Must have 1 db name to process" << endl;
				return 1;
			}

			con.OpenMode(Controller::Desktop);
			unsigned int id = con.GetDBID(dbNames[0]);
			auto_ptr<Parser> parse = GetParser(dbNames[0],filename);

			for( unsigned int i = 0; i < stCommands.size(); i++ ) {
				con.GetRecord(id, stCommands[i].index, *parse.get());

				if( stCommands[i].flag == 'r' && stCommands[i].clear ) {
					cout << "Clearing record's dirty flags..." << endl;
					con.ClearDirty(id, stCommands[i].index);
				}

				if( stCommands[i].flag == 'D' ) {
					con.DeleteRecord(id, stCommands[i].index);
				}
			}

			return 0;
		}

		// Dump contents of selected databases to stdout, or
		// to file if specified.
		// This is retrieving data from the Blackberry.
		if( dbNames.size() ) {
			vector<string>::iterator b = dbNames.begin();

			for( ; b != dbNames.end(); b++ ) {
				con.OpenMode(Controller::Desktop);
				auto_ptr<Parser> parse = GetParser(*b,filename);
				unsigned int id = con.GetDBID(*b);
				con.LoadDatabase(id, *parse.get());
			}
		}

		// Save contents of file to specified databases
		// This is writing data to the Blackberry.
		if( saveDbNames.size() ) {
			vector<string>::iterator b = saveDbNames.begin();

			for( ; b != saveDbNames.end(); b++ ) {
				con.OpenMode(Controller::Desktop);
				auto_ptr<Builder> build =
					GetBuilder(*b, filename);
				unsigned int id = con.GetDBID(*b);
				con.SaveDatabase(id, *build);
			}
		}

	}
	catch( Usb::Error &ue) {
		std::cerr << "Usb::Error caught: " << ue.what() << endl;
	}
	catch( Barry::Error &se ) {
		std::cerr << "Barry::Error caught: " << se.what() << endl;
	}
	catch( std::exception &e ) {
		std::cerr << "std::exception caught: " << e.what() << endl;
		return 1;
	}

	return 0;
}

