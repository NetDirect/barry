///
/// \file	bfuse.cc
///		FUSE filesystem for Blackberry databases, using Barry.
///

/*
    Copyright (C) 2008-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#define FUSE_USE_VERSION 25
#include <fuse.h>
#include <fuse_opt.h>

#include <barry/barry.h>
#include <sstream>
#include <getopt.h>
#include <vector>
#include <list>
#include <string>
#include <stdexcept>
#include <memory>
#include <tr1/memory>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "i18n.h"

using namespace std;
using namespace std::tr1;
using namespace Barry;

// Global filenames
const char *error_log_filename = "error.log";

// Global command line args
string cmdline_pin;
string cmdline_password;

//
// Data from the command line
//

/////////////////////////////////////////////////////////////////////////////
// Command line option handling, through fuse

//struct opt {
//	char
//};

void Blurb()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "bfuse - FUSE filesystem for Blackberry databases\n"
   << "        Copyright 2008-2009, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "        Using: " << Version << "\n"
   << endl;
}

void Usage()
{
   cerr
   << "\n"
   << "Barry specific options:\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device is plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << endl;
/*
   << "   -d db     Specify which database to mount.  If no -d options exist\n"
   << "             then all databases will be mounted.\n"
   << "             Can be used multiple times to mount more than one DB\n"
   << "   -h        This help\n"
   << "   -n        Use null parser on all databases.\n"
*/
}

/////////////////////////////////////////////////////////////////////////////
// FUSE specific exception

class fuse_error : public std::runtime_error
{
	int m_errno;
public:
	fuse_error(int errno_, const std::string &msg)
		: std::runtime_error(msg), m_errno(errno_)
	{}

	int get_errno() const { return m_errno; }
};


/////////////////////////////////////////////////////////////////////////////
// Barry record parsers

class DataDumpParser : public Barry::Parser
{
	uint32_t m_id;
	std::ostream &m_os;

public:
	explicit DataDumpParser(std::ostream &os)
		: m_os(os)
	{
	}

	virtual void Clear() {}

	virtual void SetIds(uint8_t RecType, uint32_t UniqueId)
	{
		m_id = UniqueId;
	}

	virtual void ParseHeader(const Barry::Data &, size_t &) {}

	virtual void ParseFields(const Barry::Data &data, size_t &offset,
				const Barry::IConverter *ic)
	{
		m_os << "Raw record dump for record: "
			<< std::hex << m_id << std::endl;
		m_os << data << std::endl;
	}

	virtual void Store() {}
};

template <class Record>
struct Store
{
	std::ostream &m_os;

	explicit Store(std::ostream &os)
		: m_os(os)
	{
	}

	// storage operator
	void operator()(const Record &rec)
	{
		m_os << rec;
	}
};

typedef std::auto_ptr<Barry::Parser>		ParserPtr;

ParserPtr GetParser(const string &name, std::ostream &os, bool null_parser)
{
	if( null_parser ) {
		// use null parser
		return ParserPtr( new DataDumpParser(os) );
	}
	// check for recognized database names
	else if( name == Contact::GetDBName() ) {
		return ParserPtr(
			new RecordParser<Contact, Store<Contact> > (
				new Store<Contact>(os)));
	}
	else if( name == Message::GetDBName() ) {
		return ParserPtr(
			new RecordParser<Message, Store<Message> > (
				new Store<Message>(os)));
	}
	else if( name == Calendar::GetDBName() ) {
		return ParserPtr(
			new RecordParser<Calendar, Store<Calendar> > (
				new Store<Calendar>(os)));
	}
	else if( name == ServiceBook::GetDBName() ) {
		return ParserPtr(
			new RecordParser<ServiceBook, Store<ServiceBook> > (
				new Store<ServiceBook>(os)));
	}

	else if( name == Memo::GetDBName() ) {
		return ParserPtr(
			new RecordParser<Memo, Store<Memo> > (
				new Store<Memo>(os)));
	}
	else if( name == Task::GetDBName() ) {
		return ParserPtr(
			new RecordParser<Task, Store<Task> > (
				new Store<Task>(os)));
	}
	else if( name == PINMessage::GetDBName() ) {
		return ParserPtr(
			new RecordParser<PINMessage, Store<PINMessage> > (
				new Store<PINMessage>(os)));
	}
	else if( name == SavedMessage::GetDBName() ) {
		return ParserPtr(
			new RecordParser<SavedMessage, Store<SavedMessage> > (
				new Store<SavedMessage>(os)));
	}
	else if( name == Folder::GetDBName() ) {
		return ParserPtr(
			new RecordParser<Folder, Store<Folder> > (
				new Store<Folder>(os)));
	}
	else if( name == Timezone::GetDBName() ) {
		return ParserPtr(
			new RecordParser<Timezone, Store<Timezone> > (
				new Store<Timezone>(os)));
	}
	else {
		// unknown database, use null parser
		return ParserPtr( new DataDumpParser(os) );
	}
}

/////////////////////////////////////////////////////////////////////////////
// PathSplit class

class PathSplit
{
	std::string m_pin, m_db, m_record, m_field, m_remainder;

	int m_level;		// the number of slashes, minus the first
				// i.e. first level is 0
	bool m_is_root;

public:
	explicit PathSplit(const char *path)
		: m_level(-1)
		, m_is_root(false)
	{
		if( *path != '/' )
			return;		// return in a failed state

		if( *(path+1) == 0 ) {
			m_is_root = true;
			return;
		}

		const char *s = path, *e = path;
		while( *e ) {
			while( *e && *e != '/' )
				e++;

			m_level++;

			if( s != e && (s+1) != e ) {
				string token(s+1, e);

				switch( m_level )
				{
				case 0:	// root level, should not have token here
					m_level = -1;
					return;	// failed state

				case 1:	// have pin
					m_pin = token;
					break;

				case 2:	// have db
					m_db = token;
					break;

				case 3:	// have record
					m_record = token;
					break;

				case 4:	// have field
					m_field = token;
					break;

				default:	// too many, store remainder and done
					m_remainder = s;	// keeps slash
					return;
				}

				// next
				s = e;
				if( *e )
					e++;
			}
			else if( *e ) {
				// next
				e++;
			}
		}
	}

	bool IsRoot() const { return m_is_root; }
	const std::string& Pin() const { return m_pin; }
	const std::string& DB() const { return m_db; }
	const std::string& Record() const { return m_record; }
	const std::string& Field() const { return m_field; }
	const std::string& Remainder() const { return m_remainder; }
	int Level() const { return m_level; }
};


/////////////////////////////////////////////////////////////////////////////
// API classes

class Entry
{
public:
	virtual ~Entry() {}
};

class Directory : public Entry
{
public:
	virtual int ReadDir(void *buf, fuse_fill_dir_t filler) = 0;
	virtual void FillDirStat(struct stat *st)
	{
		st->st_mode = S_IFDIR | 0555;
		st->st_nlink = 2;
	}
};

class File : public Entry
{
public:
	virtual void FillFileStat(const char *path, struct stat *st) = 0;
	virtual bool AccessOk(int flags)
	{
		// default to readonly files
		return (flags & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDONLY;
	}
	virtual int ReadFile(const char *path, char *buf, size_t size, off_t offset) = 0;
};

typedef Directory*				DirectoryPtr;
typedef File*					FilePtr;
typedef std::string				NameT;
typedef std::map<NameT, DirectoryPtr>		DirMap;
typedef std::map<NameT, FilePtr>		FileMap;

static DirMap g_dirmap;
static FileMap g_filemap;

static Directory* FindDir(const NameT &name)
{
	DirMap::iterator di = g_dirmap.find(name);
	return di == g_dirmap.end() ? 0 : di->second;
}

static File* FindFile(const NameT &name)
{
	FileMap::iterator fi = g_filemap.find(name);
	return fi == g_filemap.end() ? 0 : fi->second;
}

/////////////////////////////////////////////////////////////////////////////
// Context classes

class Database : public Directory, public File
{
public:
	Barry::Mode::Desktop &m_desk;
	std::string m_name;
	const Barry::DatabaseItem *m_pdb;

public:
	Database(Barry::Mode::Desktop &desktop,
		const std::string &pin, const Barry::DatabaseItem *pdb)
		: m_desk(desktop)
		, m_pdb(pdb)
	{
		m_name = string("/") + pin + "/" + m_pdb->Name;

		// add to directory list
		g_dirmap[ m_name ] = this;
	}

	~Database()
	{
		// remove any entries that point to us
		FileMap::iterator b = g_filemap.begin(), e = g_filemap.end();
		for( ; b != e; ++b ) {
			if( b->second == this ) {
				g_filemap.erase(b);
			}
		}

		// erase ourselves from the directory list
		g_dirmap.erase( m_name );
	}

	void AddFile(const std::string &recordId)
	{
		// FIXME - this is a hack to redirect all record files
		// to this database class... next step is to possibly
		// split out records into field files if we have a
		// parser, or just dump the hex if we don't
		string name = m_name + "/" + recordId;
		g_filemap[ name ] = this;
	}

	virtual int ReadDir(void *buf, fuse_fill_dir_t filler)
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		// list all records in database, by recordId
		Barry::RecordStateTable rst;
		m_desk.GetRecordStateTable(m_pdb->Number, rst);

		Barry::RecordStateTable::StateMapType::iterator
			b = rst.StateMap.begin(),
			e = rst.StateMap.end();
		for( ; b != e; ++ b ) {
			ostringstream oss;
			oss << hex << b->second.RecordId;
			filler(buf, oss.str().c_str(), NULL, 0);

			AddFile(oss.str());
		}
		return 0;
	}

	virtual void FillFileStat(const char *path, struct stat *st)
	{
		// use the path to find the proper record
		PathSplit ps(path);

		string constructed = string("/") + ps.Pin() + "/" + ps.DB();
		if( constructed != m_name ) {
			// FIXME - this is shoddy error handling
			throw std::logic_error("Constructed != name");
		}

		string data = GetRecordData(ps.Record());

		st->st_mode = S_IFREG | 0444;
		st->st_nlink = 1;
		st->st_size = data.size();
	}

	virtual int ReadFile(const char *path, char *buf, size_t size, off_t offset)
	{
		// use the path to find the proper record
		PathSplit ps(path);

		string constructed = string("/") + ps.Pin() + "/" + ps.DB();
		if( constructed != m_name ) {
			// FIXME - this is shoddy error handling
			throw std::logic_error("Constructed != name");
		}

		string data = GetRecordData(ps.Record());

		size_t len = data.size();
		if( offset < len ) {
			if( (offset + size) > len )
				size = len - offset;
			memcpy(buf, data.data() + offset, size);
		}
		else {
			size = 0;
		}
		return size;
	}

	const std::string& GetDBName() const { return m_pdb->Name; }

	std::string GetRecordData(const std::string &recordId)
	{
		string data;

		Barry::RecordStateTable rst;
		m_desk.GetRecordStateTable(m_pdb->Number, rst);

		uint32_t recid = strtoul(recordId.c_str(), NULL, 16);
		RecordStateTable::IndexType index;
		if( rst.GetIndex(recid, &index) ) {
			ostringstream oss;
			ParserPtr parser = GetParser(m_pdb->Name, oss, false);
			m_desk.GetRecord(m_pdb->Number, index, *parser);
			data = oss.str();
		}

		return data;
	}
};

class DesktopCon : public Directory
{
public:
	typedef std::tr1::shared_ptr<Database>			DatabasePtr;
	typedef std::list<DatabasePtr>				DBList;
public:
	Barry::Controller m_con;
	Barry::Mode::Desktop m_desk;
	std::string m_pin;
	DBList m_dblist;

	DesktopCon(const Barry::ProbeResult &result, const std::string &pin)
		: m_con(result)
		, m_desk(m_con)
		, m_pin(pin)
	{
		// add to directory list
		g_dirmap[ string("/") + pin ] = this;
	}

	~DesktopCon()
	{
		// remove from directory list
		g_dirmap.erase( string("/") + m_pin );
	}

	virtual int ReadDir(void *buf, fuse_fill_dir_t filler)
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		// list all databases in list
		DBList::const_iterator b = m_dblist.begin(), e = m_dblist.end();
		for( ; b != e; ++ b ) {
			filler(buf, (*b)->GetDBName().c_str(), NULL, 0);
		}
		return 0;
	}

	void Open(const char *password = 0)
	{
		// open our device
		m_desk.Open(password);

		// add all databases as directories
		DatabaseDatabase::DatabaseArrayType::const_iterator
			dbi = m_desk.GetDBDB().Databases.begin(),
			dbe = m_desk.GetDBDB().Databases.end();
		for( ; dbi != dbe; ++dbi ) {
			DatabasePtr db = DatabasePtr(
				new Database(m_desk, m_pin, &(*dbi)) );
			m_dblist.push_back(db);
		}
	}
};

class Context : public Directory, public File
{
public:
	typedef std::auto_ptr<Barry::Probe>			ProbePtr;
	typedef std::tr1::shared_ptr<DesktopCon>		DesktopConPtr;
	typedef std::string					PinT;
	typedef std::map<PinT, DesktopConPtr>			PinMap;

	ProbePtr m_probe;
	PinMap m_pinmap;

	string m_error_log;

	string m_limit_pin;		// only mount device with this pin
	string m_password;		// use this password when connecting

public:
	Context(const string &limit_pin = "", const string &password = "")
		: m_limit_pin(limit_pin)
		, m_password(password)
	{
		g_dirmap["/"] = this;
		g_filemap[string("/") + error_log_filename] = this;

		m_error_log = "Hello FUSE world.  This is Barry.  Pleased to meet you.\n";
	}

	~Context()
	{
		g_dirmap.erase("/");
		g_filemap.erase(string("/") + error_log_filename);
	}

	virtual int ReadDir(void *buf, fuse_fill_dir_t filler)
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		filler(buf, error_log_filename, NULL, 0);

		// list all pins in map
		PinMap::const_iterator b = m_pinmap.begin(), e = m_pinmap.end();
		for( ; b != e; ++ b ) {
			filler(buf, b->first.c_str(), NULL, 0);
		}
		return 0;
	}

	virtual void FillFileStat(const char *path, struct stat *st)
	{
		st->st_mode = S_IFREG | 0444;
		st->st_nlink = 1;
		st->st_size = m_error_log.size();
	}

	virtual int ReadFile(const char *path, char *buf, size_t size, off_t offset)
	{
		size_t len = m_error_log.size();
		if( offset < len ) {
			if( (offset + size) > len )
				size = len - offset;
			memcpy(buf, m_error_log.data() + offset, size);
		}
		else {
			size = 0;
		}
		return size;
	}

	void Log(const std::string &msg)
	{
		m_error_log += msg;
		m_error_log += "\n";
	}

	const std::string& GetLog() const { return m_error_log; }

	void ProbeAll()
	{
		// probe the USB bus for Blackberry devices
		m_probe.reset( new Probe );

		// connect to all PINs found, and add them to our map
		for( int i = 0; i < m_probe->GetCount(); i++ ) {
			string curpin = m_probe->Get(i).m_pin.str();

			// don't add a blank or pre-existing pin
			if( !curpin.size() || m_pinmap.find(curpin) != m_pinmap.end() ) {
				continue;
			}

			// don't add non-PIN device if pin specified
			if( m_limit_pin.size() && curpin != m_limit_pin ) {
				continue;
			}

			DesktopConPtr dev = DesktopConPtr (
				new DesktopCon(m_probe->Get(i), curpin) );
			dev->Open(m_password.c_str());
			m_pinmap[ curpin ] = dev;
		}
	}

	DesktopCon* FindPin(PinT pin)
	{
		PinMap::iterator pi = m_pinmap.find(pin);
		return pi == m_pinmap.end() ? 0 : pi->second.get();
	}
};


/////////////////////////////////////////////////////////////////////////////
// FUSE API hooks

static void* bfuse_init()
{
	// Initialize the barry library.  Must be called before
	// anything else.
	Barry::Init(false);

	Context *ctx = 0;

	try {
		ctx = new Context(cmdline_pin, cmdline_password);
		ctx->ProbeAll();
	}
	catch( std::exception &e ) {
		if( ctx ) {
			ctx->Log(e.what());
		}
	}

	return ctx;
}

static void bfuse_destroy(void *data)
{
	if( data ) {
		Context *ctx = (Context*) data;
		delete ctx;
	}
}

static int bfuse_getattr(const char *path, struct stat *st)
{
	memset(st, 0, sizeof(*st));

	if( Directory *dir = FindDir(path) ) {
		dir->FillDirStat(st);
		return 0;
	}
	else if( File *file = FindFile(path) ) {
		file->FillFileStat(path, st);
		return 0;
	}
	else
		return -ENOENT;
}

static int bfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t /*offset*/, struct fuse_file_info * /*fi*/)
{
	Directory *dir = FindDir(path);
	if( !dir )
		return -ENOENT;
	return dir->ReadDir(buf, filler);
}

static int bfuse_open(const char *path, struct fuse_file_info *fi)
{
	File *file = FindFile(path);
	if( !file )
		return -ENOENT;

	if( !file->AccessOk(fi->flags) )
		return -EACCES;

	return 0;
}

static int bfuse_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	File *file = FindFile(path);
	if( !file )
		return -ENOENT;

	return file->ReadFile(path, buf, size, offset);
}

// static struct here automatically zeros data
static struct fuse_operations bfuse_oper;


/////////////////////////////////////////////////////////////////////////////
// main

int main(int argc, char *argv[])
{
	INIT_I18N(PACKAGE);

	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	Blurb();

	// initialize the operation hooks
	bfuse_oper.init		= bfuse_init;
	bfuse_oper.destroy	= bfuse_destroy;
	bfuse_oper.getattr	= bfuse_getattr;
	bfuse_oper.readdir	= bfuse_readdir;
	bfuse_oper.open		= bfuse_open;
	bfuse_oper.read		= bfuse_read;

	// process command line options before FUSE does
	// FUSE does its own command line processing, and
	// doesn't seem to have a way to plug into it,
	// so do our own first
	int fuse_argc = 0;
	char **fuse_argv = new char*[argc];

	for( int i = 0; i < argc; i++ ) {
		if( argv[i][0] == '-' ) {

			switch( argv[i][1] )
			{
//			case 'd':	// mount dbname
//				dbNames.push_back(string(optarg));
//				break;

//			case 'n':	// use null parser
//				null_parser = true;
//				break;

			case 'p':	// Blackberry PIN
				if( i+1 < argc ) {
					cmdline_pin = argv[++i];
				}
				continue;

			case 'P':	// Device password
				if( i+1 < argc ) {
					cmdline_password = argv[++i];
				}
				continue;

			case 'h':	// help
				Usage();
				break;
			}
		}

		// if we get here, add this option to FUSE's
		fuse_argv[fuse_argc] = argv[i];
		fuse_argc++;
	}

	int ret = fuse_main(fuse_argc, fuse_argv, &bfuse_oper);
	delete [] fuse_argv;
	return ret;
}

