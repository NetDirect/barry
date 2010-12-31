///
/// \file	bio.cc
///		Barry Input / Output
///

/*
    Copyright (C) 2010, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <barry/barrysync.h>
#include <barry/barrybackup.h>

#include "mimedump.h"
#include "brecsum.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <tr1/memory>
#include <getopt.h>
#include <strings.h>

using namespace std;
using namespace std::tr1;
using namespace Barry;

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "bio - Barry Input / Output\n"
   << "      Copyright 2010, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "      Using: " << Version << "\n"
   << "      Compiled "
#ifdef __BARRY_BOOST_MODE__
   << "with"
#else
   << "without"
#endif
   << " Boost support\n"
   << "\n"
   << " Usage:  bio -i <type> [options...]   -o <type> [options...]\n"
   << "\n"
   << "   -i type   The input type (Builder) to use for producing records\n"
   << "             Can be one of: device, tar"
#ifdef __BARRY_BOOST_MODE__
   << ", boost"
#endif
   << ", ldif, mime\n"
   << "   -o type   The output type (Parser) to use for processing records.\n"
   << "             Multiple outputs are allowed, as long as they don't\n"
   << "             conflict (such as two outputs writing to the same file\n"
   << "             or device).\n"
   << "             Can be one of: device, tar"
#ifdef __BARRY_BOOST_MODE__
   << ", boost"
#endif
   << ", ldif, mime, dump, sha1, cstore\n"
   << "\n"
   << " Options to use for 'device' type:\n"
   << "   -d db     Name of input database. Can be used multiple times.\n"
   << "   -A        Add all available device databases, instead of specifying\n"
   << "             them manually via -d\n"
   << "   -p pin    PIN of device to talk to\n"
   << "             If only one device is plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << "   -w mode   Set write mode when using 'device' for output.  Must be\n"
   << "             specified, or will not write anything.\n"
   << "             Can be one of: erase, overwrite, addonly, addnew\n"
/*
// FIXME - modifiers not yet implemented
   << "\n"
   << " Input database modifiers:  (can be used multiple times for more than 1 record)\n"
   << "\n"
   << "   -r #      Record index number as seen in the -T state table.\n"
   << "             This overrides the default -d behaviour, and only\n"
   << "             downloads the one specified record, sending to stdout.\n"
   << "   -R #      Same as -r, but also clears the record's dirty flags.\n"
   << "   -D #      Record index number as seen in the -T state table,\n"
   << "             which indicates the record to delete.  Used with the -d\n"
   << "             command to specify the database.\n"
*/
   << "\n"
   << " Options to use for 'tar' backup type:\n"
   << "   -d db     Name of input database. Can be used multiple times.\n"
   << "             Not available in output mode.  Note that by default,\n"
   << "             all databases in the backup are selected, when reading,\n"
   << "             unless at least one -d is specified.\n"
   << "   -f file   Tar backup file to read from or write to\n"
#ifdef __BARRY_BOOST_MODE__
   << "\n"
   << " Options to use for 'boost' type:\n"
   << "   -f file   Boost serialization filename to read from or write to\n"
   << "             Can use - to specify stdin/stdout\n"
#endif
   << "\n"
   << " Options to use for 'ldif' type:\n"
   << "   -c dn     Convert address book database to LDIF format, using the\n"
   << "             specified baseDN\n"
   << "   -C dnattr LDIF attribute name to use when building the FQDN\n"
   << "             Defaults to 'cn'\n"
/*
LDIF options?

   << "   -L        List Contact field names\n"
   << "   -m        Map LDIF name to Contact field / Unmap LDIF name\n"
   << "                Map: ldif,read,write - maps ldif to read/write Contact fields\n"
   << "                Unmap: ldif name alone\n"
   << "   -M        List current LDIF mapping\n"
*/
   << "\n"
   << " Options to use for 'mime' type:\n"
   << "   -f file   Filename to read from or write to.  Use - to explicitly\n"
   << "             specify stdin/stdout, which is default.\n"
   << "\n"
   << " Options to use for 'dump' to stdout output type:\n"
   << "   -n        Use hex dump parser on all databases.\n"
   << "\n"
   << " Options to use for 'sha1' sum stdout output type:\n"
   << "   -t        Include DB Name, Type, and Unique record IDs in the checksums\n"
   << "\n"
   << " Options to use for 'cstore' output type:\n"
   << "   -l        List filenames only\n"
   << "   -f file   Filename from the above list, including path.\n"
   << "             If found, the file will be written to the current\n"
   << "             directory, using the base filename from the device.\n"
   << "\n"
   << " Standalone options:\n"
   << "   -h        This help\n"
   << "   -I cs     International charset for string conversions\n"
   << "             Valid values here are available with 'iconv --list'\n"
   << "   -S        Show list of supported database parsers and builders\n"
   << "   -v        Dump protocol data during operation\n"
   << "\n"
   << endl;
}

class ModeBase
{
public:
	virtual ~ModeBase() {}

	virtual bool ProbeNeeded() const { return false; }

	virtual void SetFilename(const std::string &name)
	{
		throw runtime_error("Filename not applicable for this mode");
	}

	virtual void AddDB(const std::string &dbname)
	{
		throw runtime_error("DB not applicable for this mode");
	}

	virtual void AddAllDBs()
	{
		throw runtime_error("DBs not applicable for this mode");
	}

	virtual void SetPIN(const std::string &pin)
	{
		throw runtime_error("PIN not applicable for this mode");
	}

	virtual void SetPassword(const std::string &password)
	{
		throw runtime_error("Password not applicable for this mode");
	}

	virtual void SetWriteMode(DeviceParser::WriteMode mode)
	{
		throw runtime_error("Device write behaviour not applicable for this mode");
	}

	virtual void SetDN(const std::string &dn)
	{
		throw runtime_error("DN not applicable for this mode");
	}

	virtual void SetAttribute(const std::string &attr)
	{
		throw runtime_error("Attribute not applicable for this mode");
	}

	virtual void SetHexDump()
	{
		throw runtime_error("No hex dump option in this mode");
	}

	virtual void IncludeIDs()
	{
		throw runtime_error("Including record IDs in the SHA1 sum is not applicable in this mode");
	}

	virtual void SetList()
	{
		throw runtime_error("List option not applicable for this mode");
	}
};

class DeviceBase : public virtual ModeBase
{
protected:
	Barry::Pin m_pin;
	std::string m_password;

public:
	bool ProbeNeeded() const { return true; }

	void SetPIN(const std::string &pin)
	{
		istringstream iss(pin);
		iss >> m_pin;
		if( !m_pin.Valid() )
			throw runtime_error("Invalid PIN: " + pin);
	}

	void SetPassword(const std::string &password)
	{
		m_password = password;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Base class for Input Mode

class InputBase : public virtual ModeBase
{
public:
	virtual Builder& GetBuilder(Barry::Probe *probe, IConverter &ic) = 0;
};

class DeviceInputBase : public DeviceBase, public InputBase
{
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Input, Type: device

class DeviceInput : public DeviceInputBase
{
	auto_ptr<Controller> m_con;
	auto_ptr<Mode::Desktop> m_desktop;
	auto_ptr<DeviceBuilder> m_builder;
	vector<string> m_dbnames;
	bool m_add_all;

public:
	DeviceInput()
		: m_add_all(false)
	{
	}

	void AddDB(const std::string &dbname)
	{
		m_dbnames.push_back(dbname);
	}

	void AddAllDBs()
	{
		m_add_all = true;
	}

	Builder& GetBuilder(Barry::Probe *probe, IConverter &ic)
	{
		int i = probe->FindActive(m_pin);
		if( i == -1 ) {
			if( m_pin.Valid() )
				throw runtime_error("PIN not found: " + m_pin.Str());
			else
				throw runtime_error("PIN not specified, and more than one device exists.");
		}

		m_con.reset( new Controller(probe->Get(i)) );
		m_desktop.reset( new Mode::Desktop(*m_con, ic) );
		m_desktop->Open(m_password.c_str());
		m_builder.reset( new DeviceBuilder(*m_desktop) );

		if( m_add_all ) {
			m_builder->Add(m_desktop->GetDBDB());
		}
		else {
			for( size_t i = 0; i < m_dbnames.size(); i++ ) {
				m_builder->Add(m_dbnames[i]);
			}
		}

		return *m_builder;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Input, Type: tar

class TarInput : public InputBase
{
	auto_ptr<Restore> m_restore;
	string m_tarpath;
	vector<string> m_dbnames;

public:
	void SetFilename(const std::string &name)
	{
		m_tarpath = name;
		if( name == "-" )
			throw runtime_error("Cannot use stdin as tar source file, sorry.");
	}

	void AddDB(const std::string &dbname)
	{
		m_dbnames.push_back(dbname);
	}

	Builder& GetBuilder(Barry::Probe *probe, IConverter &ic)
	{
		m_restore.reset( new Restore(m_tarpath, true) );
		for( size_t i = 0; i < m_dbnames.size(); i++ ) {
			m_restore->AddDB(m_dbnames[i]);
		}

		return *m_restore;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Input, Type: boost

#ifdef __BARRY_BOOST_MODE__
class BoostInput : public InputBase
{
	auto_ptr<BoostBuilder> m_builder;
	string m_filename;

public:
	BoostInput()
		: m_filename("-")	// default to stdin/stdout
	{
	}

	void SetFilename(const std::string &name)
	{
		m_filename = name;
	}

	Builder& GetBuilder(Barry::Probe *probe, IConverter &ic)
	{
		if( m_filename == "-" ) {
			// use stdin
			m_builder.reset( new BoostBuilder(cin) );
		}
		else {
			m_builder.reset( new BoostBuilder(m_filename) );
		}
		return *m_builder;
	}

};
#endif

//////////////////////////////////////////////////////////////////////////////
// Mode: Input, Type: ldif

class LdifInput : public InputBase
{
	auto_ptr<Builder> m_builder;
	string m_filename;

public:
	LdifInput()
		: m_filename("-")
	{
	}

	void SetFilename(const std::string &name)
	{
		m_filename = name;
	}

	Builder& GetBuilder(Barry::Probe *probe, IConverter &ic)
	{
		if( m_filename == "-" ) {
			// use stdin
			m_builder.reset(
				new RecordBuilder<Contact, LdifStore>(
					new LdifStore(cin)) );
		}
		else {
			m_builder.reset(
				new RecordBuilder<Contact, LdifStore>(
					new LdifStore(m_filename)) );
		}
		return *m_builder;
	}

};


//////////////////////////////////////////////////////////////////////////////
// Mode: Input, Type: mime

class MimeInput : public InputBase
{
	auto_ptr<MimeBuilder> m_builder;
	string m_filename;

public:
	MimeInput()
		: m_filename("-")
	{
	}

	void SetFilename(const std::string &name)
	{
		m_filename = name;
	}

	Builder& GetBuilder(Barry::Probe *probe, IConverter &ic)
	{
		if( m_filename == "-" ) {
			// use stdin
			m_builder.reset( new MimeBuilder(cin) );
		}
		else {
			m_builder.reset( new MimeBuilder(m_filename) );
		}
		return *m_builder;
	}

};

//////////////////////////////////////////////////////////////////////////////
// Base class for Output Mode

class OutputBase : public virtual ModeBase
{
public:
	virtual Parser& GetParser(Barry::Probe *probe, IConverter &ic) = 0;
};

class DeviceOutputBase : public DeviceBase, public OutputBase
{
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Output, Type: device

class DeviceOutput : public DeviceOutputBase
{
	auto_ptr<Controller> m_con;
	auto_ptr<Mode::Desktop> m_desktop;
	auto_ptr<DeviceParser> m_parser;
	DeviceParser::WriteMode m_mode;

public:
	DeviceOutput()
		: m_mode(DeviceParser::DROP_RECORD)
	{
	}

	void SetWriteMode(DeviceParser::WriteMode mode)
	{
		m_mode = mode;
	}

	Parser& GetParser(Barry::Probe *probe, IConverter &ic)
	{
		int i = probe->FindActive(m_pin);
		if( i == -1 ) {
			if( m_pin.Valid() )
				throw runtime_error("PIN not found: " + m_pin.Str());
			else
				throw runtime_error("PIN not specified, and more than one device exists.");
		}

		m_con.reset( new Controller(probe->Get(i)) );
		m_desktop.reset( new Mode::Desktop(*m_con, ic) );
		m_desktop->Open(m_password.c_str());
		m_parser.reset( new DeviceParser(*m_desktop, m_mode) );

		return *m_parser;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Output, Type: tar

class TarOutput : public OutputBase
{
	auto_ptr<Backup> m_backup;
	string m_tarpath;

public:
	void SetFilename(const std::string &name)
	{
		m_tarpath = name;
		if( name == "-" )
			throw runtime_error("Cannot use stdout as tar backup file, sorry.");
	}

	Parser& GetParser(Barry::Probe *probe, IConverter &ic)
	{
		m_backup.reset( new Backup(m_tarpath) );
		return *m_backup;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Output, Type: boost

#ifdef __BARRY_BOOST_MODE__
class BoostOutput : public OutputBase
{
	auto_ptr<BoostParser> m_parser;
	string m_filename;

public:
	void SetFilename(const std::string &name)
	{
		m_filename = name;
	}

	Parser& GetParser(Barry::Probe *probe, IConverter &ic)
	{
		if( !m_filename.size() )
			throw runtime_error("Boost output requires a specific output file (-f switch)");

		if( m_filename == "-" ) {
			// use stdout
			m_parser.reset( new BoostParser(cout) );
		}
		else {
			m_parser.reset( new BoostParser(m_filename) );
		}
		return *m_parser;
	}

};
#endif

//////////////////////////////////////////////////////////////////////////////
// Mode: Output, Type: ldif

class LdifOutput : public OutputBase
{
	auto_ptr<Parser> m_parser;
	string m_filename;
	string m_baseDN;
	string m_dnattr;

public:
	LdifOutput()
		: m_filename("-")
	{
	}

	void SetFilename(const std::string &name)
	{
		m_filename = name;
	}

	void SetDN(const std::string &dn)
	{
		m_baseDN = dn;
	}

	void SetAttribute(const std::string &attr)
	{
		m_dnattr = attr;
	}

	Parser& GetParser(Barry::Probe *probe, IConverter &ic)
	{
		if( m_filename == "-" ) {
			// use stdin
			m_parser.reset(
				new RecordParser<Contact, LdifStore>(
					new LdifStore(cout, m_baseDN,
						m_dnattr)) );
		}
		else {
			m_parser.reset(
				new RecordParser<Contact, LdifStore>(
					new LdifStore(m_filename, m_baseDN,
						m_dnattr)) );
		}
		return *m_parser;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Output, Type: mime

class MimeStore : public AllRecordStore
{
	std::ostream &m_os;

public:
	MimeStore(std::ostream &os)
		: m_os(os)
	{
	}

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	void operator() (const Barry::tname &r) \
	{ \
		MimeDump<tname>::Dump(m_os, r); \
	}

	ALL_KNOWN_PARSER_TYPES
};

class MimeOutput : public OutputBase
{
	auto_ptr<std::ofstream> m_file;
	auto_ptr<Parser> m_parser;
	std::string m_filename;

public:
	MimeOutput()
		: m_filename("-")	// default to stdout
	{
	}

	void SetFilename(const std::string &name)
	{
		m_filename = name;
	}

	Parser& GetParser(Barry::Probe *probe, IConverter &ic)
	{
		if( m_filename == "-" ) {
			m_parser.reset( new AllRecordParser(cout,
				new HexDumpParser(cout),
				new MimeStore(cout)) );
		}
		else {
			m_file.reset( new std::ofstream(m_filename.c_str()) );
			m_parser.reset( new AllRecordParser(*m_file,
				new HexDumpParser(*m_file),
				new MimeStore(*m_file)) );
		}
		return *m_parser;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Output, Type: dump

class DumpOutput : public OutputBase
{
	auto_ptr<Parser> m_parser;
	bool m_hex_only;

public:
	DumpOutput()
		: m_hex_only(false)
	{
	}

	void SetHexDump()
	{
		m_hex_only = true;
	}

	Parser& GetParser(Barry::Probe *probe, IConverter &ic)
	{
		if( m_hex_only ) {
			m_parser.reset( new HexDumpParser(cout) );
		}
		else {
			m_parser.reset( new AllRecordParser(cout,
				new HexDumpParser(cout),
				new AllRecordDumpStore(cout)) );
		}
		return *m_parser;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Output, Type: sha1

class Sha1Output : public OutputBase
{
	auto_ptr<Parser> m_parser;
	bool m_include_ids;

public:
	Sha1Output()
		: m_include_ids(false)
	{
	}

	void IncludeIDs()
	{
		m_include_ids = true;
	}

	Parser& GetParser(Barry::Probe *probe, IConverter &ic)
	{
		m_parser.reset( new ChecksumParser(m_include_ids) );
		return *m_parser;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Mode: Output, Type: cstore

class ContentStoreOutput : public OutputBase
{
	auto_ptr<Parser> m_parser;
	bool m_list_only;
	vector<string> m_filenames;

public:
	ContentStoreOutput()
		: m_list_only(false)
	{
	}

	void SetFilename(const std::string &name)
	{
		m_filenames.push_back(name);
	}

	void SetList()
	{
		m_list_only = true;
	}

	Parser& GetParser(Barry::Probe *probe, IConverter &ic)
	{
		m_parser.reset( new RecordParser<ContentStore, ContentStoreOutput>(*this) );
		return *m_parser;
	}

	// storage operator
	void operator() (const ContentStore &rec)
	{
		if( m_list_only ) {
			cout << rec.Filename;
			if( rec.FolderFlag ) {
				cout << " (folder)";
			}
			cout << endl;
		}
		else {
			// check if this record matches one of the filenames
			// in the list
			vector<string>::iterator i = find(m_filenames.begin(),
				m_filenames.end(), rec.Filename);
			if( i != m_filenames.end() ) {
				SaveFile(rec);
			}
		}
	}

	void SaveFile(const ContentStore &rec)
	{
		size_t slash = rec.Filename.rfind('/');
		string filename;
		if( slash == string::npos )
			filename = rec.Filename;
		else
			filename = rec.Filename.substr(slash + 1);

		// modify filename until we find one that doesn't
		// already exist
		string freshname = filename;
		int count = 0;
		while( access(freshname.c_str(), F_OK) == 0 ) {
			ostringstream oss;
			oss << filename << count++;
			freshname = oss.str();
		}

		// open and write!
		cout << "Saving: " << rec.Filename
			<< " as " << freshname << endl;
		ofstream ofs(freshname.c_str());
		ofs << rec.FileContent;
		ofs.flush();
		if( !ofs ) {
			cout << "Error during write!" << endl;
		}
	}
};



//////////////////////////////////////////////////////////////////////////////
// Main application class

class App
{
public:
	typedef shared_ptr<OutputBase> OutputPtr;
	typedef vector<OutputPtr> OutputsType;

private:
	auto_ptr<InputBase> Input;
	OutputsType Outputs;

public:

	bool ParseInMode(const string &mode);
	bool ParseOutMode(const string &mode);
	DeviceParser::WriteMode ParseWriteMode(const std::string &mode);
	static void ShowParsers();
	// returns true if any of the items in Outputs needs a probe
	bool OutputsProbeNeeded();
	int main(int argc, char *argv[]);
};

bool App::ParseInMode(const string &mode)
{
	if( mode == "device" ) {
		Input.reset( new DeviceInput );
		return true;
	}
	else if( mode == "tar" ) {
		Input.reset( new TarInput );
		return true;
	}
#ifdef __BARRY_BOOST_MODE__
	else if( mode == "boost" ) {
		Input.reset( new BoostInput );
		return true;
	}
#endif
	else if( mode == "ldif" ) {
		Input.reset( new LdifInput );
		return true;
	}
	else if( mode == "mime" ) {
		Input.reset( new MimeInput );
		return true;
	}
	else
		return false;
}

bool App::ParseOutMode(const string &mode)
{
	if( mode == "device" ) {
		Outputs.push_back( OutputPtr(new DeviceOutput) );
		return true;
	}
	else if( mode == "tar" ) {
		Outputs.push_back( OutputPtr(new TarOutput) );
		return true;
	}
#ifdef __BARRY_BOOST_MODE__
	else if( mode == "boost" ) {
		Outputs.push_back( OutputPtr(new BoostOutput) );
		return true;
	}
#endif
	else if( mode == "ldif" ) {
		Outputs.push_back( OutputPtr(new LdifOutput) );
		return true;
	}
	else if( mode == "mime" ) {
		Outputs.push_back( OutputPtr(new MimeOutput) );
		return true;
	}
	else if( mode == "dump" ) {
		Outputs.push_back( OutputPtr(new DumpOutput) );
		return true;
	}
	else if( mode == "sha1" ) {
		Outputs.push_back( OutputPtr(new Sha1Output) );
		return true;
	}
	else if( mode == "cstore" ) {
		Outputs.push_back( OutputPtr(new ContentStoreOutput) );
		return true;
	}
	else
		return false;
}

DeviceParser::WriteMode App::ParseWriteMode(const std::string &mode)
{
	if( mode == "erase" )
		return DeviceParser::ERASE_ALL_WRITE_ALL;
	else if( mode == "overwrite" )
		return DeviceParser::INDIVIDUAL_OVERWRITE;
	else if( mode == "addonly" )
		return DeviceParser::ADD_BUT_NO_OVERWRITE;
	else if( mode == "addnew" )
		return DeviceParser::ADD_WITH_NEW_ID;
	else
		throw runtime_error("Unknown device output mode. Must be one of: erase, overwrite, addonly, addnew");
}

void App::ShowParsers()
{
	cout << "Supported Database parsers:\n"
	<< " (* = can display in vformat MIME mode)\n"

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	<< "   " << tname::GetDBName() \
		<< (MimeDump<tname>::Supported() ? " *" : "") << "\n"

	ALL_KNOWN_PARSER_TYPES

	<< "\n"
	<< "Supported Database builders:\n"

#undef HANDLE_BUILDER
#define HANDLE_BUILDER(tname) \
	<< "   " << tname::GetDBName() << "\n"

	ALL_KNOWN_BUILDER_TYPES

	<< endl;
}

bool App::OutputsProbeNeeded()
{
	for( OutputsType::iterator i = Outputs.begin();
		i != Outputs.end();
		++i )
	{
		if( (*i)->ProbeNeeded() )
			return true;
	}
	return false;
}

int App::main(int argc, char *argv[])
{
	bool verbose = false;
	string iconvCharset;

	// process command line options
	ModeBase *current = 0;
	for(;;) {
		int cmd = getopt(argc, argv, "hi:o:nvI:f:p:P:d:c:C:ASw:tl");
		if( cmd == -1 )
			break;

		// first option must be in or out, or a global option
		if( !current ) {
			if( cmd != 'i' && \
			    cmd != 'o' && \
			    cmd != 'S' && \
			    cmd != 'I' && \
			    cmd != 'v' )
			{
				Usage();
				return 1;
			}
		}

		switch( cmd )
		{
		case 'i':	// set input mode
			// must be first time used
			if( Input.get() || !ParseInMode(optarg) ) {
				Usage();
				return 1;
			}
			current = Input.get();
			break;

		case 'o':	// set output mode
			// can be used multiple times
			if( !ParseOutMode(optarg) ) {
				Usage();
				return 1;
			}
			current = Outputs[Outputs.size() - 1].get();
			break;


		case 'c':	// set ldif dn
			current->SetDN(optarg);
			break;

		case 'C':	// set ldif attr
			current->SetAttribute(optarg);
			break;

		case 'd':	// database name
			current->AddDB(optarg);
			break;

		case 'f':	// filename
			current->SetFilename(optarg);
			break;

		case 'p':	// device PIN
			current->SetPIN(optarg);
			break;

		case 'P':	// password
			current->SetPassword(optarg);
			break;

		case 'w':	// device write mode
			current->SetWriteMode(ParseWriteMode(optarg));
			break;

		case 'A':	// add all DB names to the device builder
			current->AddAllDBs();
			break;

		case 't':	// include type and IDs in sha1 mode
			current->IncludeIDs();
			break;

		case 'l':	// list only
			current->SetList();
			break;

		case 'S':	// show parsers and builders
			ShowParsers();
			return 0;

		case 'I':	// international charset (iconv)
			iconvCharset = optarg;
			break;

		case 'n':	// use null hex dump parser only
			current->SetHexDump();
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

	if( !Input.get() || !Outputs.size() ) {
		Usage();
		return 0;
	}

	// Initialize the Barry library
	Barry::Init(verbose);

	// Create an IConverter object if needed
	auto_ptr<IConverter> ic;
	if( iconvCharset.size() ) {
		ic.reset( new IConverter(iconvCharset.c_str(), true) );
	}

	// Probe for devices only if needed
	auto_ptr<Probe> probe;
	if( Input->ProbeNeeded() || OutputsProbeNeeded() ) {
		// Probe for available devices
		probe.reset( new Probe );
	}

	// Setup the input first (builder)
	Builder &builder = Input->GetBuilder(probe.get(), *ic);

	// Setup a TeeParser with all Outputs
	TeeParser tee;
	for( OutputsType::iterator i = Outputs.begin(); i != Outputs.end(); ++i ) {
		Parser &parser = (*i)->GetParser(probe.get(), *ic);
		tee.Add(parser);
	}

	// Setup the pipe
	Pipe pipe(builder);
	pipe.PumpFile(tee, ic.get());

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

