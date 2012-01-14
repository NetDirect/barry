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
#include <tr1/memory>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>

#include "barrygetopt.h"

using namespace std;
using namespace std::tr1;
using namespace Barry;

/*
- write btarcmp tool to compare two barry backup tarball files and report
        differences...
                - report differences in a concise list (uniqueId and displayname
                  or something), with an option for verbose
                - modes of comparison:
                        - binary compare (similar to brecsum, with its options)
                        - or parsed record compare, with list of fields that
                          are different
                - also, should have the ability to copy all differing records
                        into another tarball, to function as a "patch", so
                        user can write it to his device to update only differing
                        records, or to store as a record of changes, etc.
*/
void Usage()
{
   int logical, major, minor;
   const char *Version = Barry::Version(logical, major, minor);

   cerr
   << "btarcmp - Compare Barry backup tarballs\n"
   << "      Copyright 2012, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "      Using: " << Version << "\n"
   << "\n"
   << " Usage:  btarcmp [options...] tarball_0 tarball_1\n"
   << "\n"
   << "   -b        Use brief output\n"
   << "   -d db     Specify a specific database to compare.  Can be used\n"
   << "             multiple times.  If not used at all, all databases are\n"
   << "             compared.\n"
   << "   -D db     Specify a database name to skip.  If both -d and -D are\n"
   << "             used for the same database name, it will be skipped.\n"
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
public:
	typedef Barry::ConfigFile::DBListType		DBListType;
	typedef std::vector<Barry::DBData>		DBDataList;
	typedef std::map<std::string, DBDataList>	DatabaseMap;

private:
	DBListType m_compare_list;
	DBListType m_skip_list;
	DBListType m_valid_list;	// this list is created during the
					// database name compare... it holds
					// all the names that exist in both
					// maps, in sorted order
	DatabaseMap m_tars[2];
	std::string m_tarpaths[2];	// full filename with path
	std::string m_tarfiles[2];	// just filename, no path; or brief mark
	auto_ptr<IConverter> m_ic;

	int m_main_return;
	bool m_verbose;
	std::string m_last_dbname;

public:
	App();

	void LoadTarballs();
	void CompareDatabaseNames();
	void CompareData();
	void Compare(const std::string &dbname);
	void Compare(const std::string &dbname,
		const DBDataList &one, const DBDataList &two);
	void Compare(const DBData &one, const DBData &two);

	bool Alike(DBDataList::const_iterator b1, DBDataList::const_iterator b2,
		DBDataList::const_iterator e1, DBDataList::const_iterator e2);
	void SearchCheck(DBDataList::const_iterator &b,
		DBDataList::const_iterator &e, const DBDataList &opposite_list,
		const std::string &action);

	void DumpRecord(const DBData &data);
	void ShowDatabaseHeader(const std::string &dbname);

	// returns true if any of the items in Outputs needs a probe
	int main(int argc, char *argv[]);

	static void ShowParsers();
};

//////////////////////////////////////////////////////////////////////////////
// Memory storage parser

class StoreParser : public Barry::Parser
{
	App::DatabaseMap &m_map;

public:
	explicit StoreParser(App::DatabaseMap &map)
		: m_map(map)
	{
	}

	virtual void ParseRecord(const DBData &data, const IConverter *ic)
	{
		m_map[data.GetDBName()].push_back(data);
	}
};


//////////////////////////////////////////////////////////////////////////////
// Member function definitions

App::App()
	: m_main_return(0)
	, m_verbose(false)
{
}

void App::ShowParsers()
{
	cout << "Supported Database parsers:\n"

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	<< "   " << tname::GetDBName() << "\n"

	ALL_KNOWN_PARSER_TYPES

	<< endl;
}

bool DBDataCmp(const DBData &a, const DBData &b)
{
	return a.GetUniqueId() < b.GetUniqueId();
}

class DBDataIdCmp
{
	uint32_t m_id;

public:
	explicit DBDataIdCmp(uint32_t id)
		: m_id(id)
	{
	}

	bool operator()(const DBData &data) const
	{
		return data.GetUniqueId() == m_id;
	}
};

bool IdExists(const App::DBDataList &list, uint32_t id)
{
	return find_if(list.begin(), list.end(), DBDataIdCmp(id)) != list.end();
}

void App::LoadTarballs()
{
	for( int i = 0; i < 2; i++ ) {
		// load data into memory
		Restore builder(m_tarpaths[i]);
		StoreParser parser(m_tars[i]);

		Pipe pipe(builder);
		pipe.PumpFile(parser, m_ic.get());

		// sort each database's record data by UniqueId
		for( DatabaseMap::iterator b = m_tars[i].begin();
			b != m_tars[i].end();
			++b )
		{
			sort(b->second.begin(), b->second.end(), DBDataCmp);
		}
	}
}

void App::CompareDatabaseNames()
{
	for( int i = 1; i >= 0; i-- ) {
		int other = i == 0 ? 1 : 0;

		DatabaseMap::const_iterator b = m_tars[i].begin(), match;
		for( ; b != m_tars[i].end(); ++b ) {
			match = m_tars[other].find(b->first);
			if( match == m_tars[other].end() ) {
				cout << m_tarfiles[other] << ": has no database '" << b->first << "'" << endl;
				m_main_return = 2;
			}
			else {
				if( !m_valid_list.IsSelected(b->first) ) {
					m_valid_list.push_back(b->first);
				}
			}
		}
	}

	// sort the valid list
	sort(m_valid_list.begin(), m_valid_list.end());
//	cout << m_valid_list.size() << " valid database names found." << endl;
}

void App::CompareData()
{
	DBListType::const_iterator valid = m_valid_list.begin();
	for( ; valid != m_valid_list.end(); ++valid ) {
		// if m_compare_list contains items, then only compare
		// if this database is present in the list
		if( m_compare_list.size() && !m_compare_list.IsSelected(*valid) )
			continue;

		// check if we should skip this database
		if( m_skip_list.IsSelected(*valid) )
			continue;

		// all's well so far... compare!
		Compare(*valid);
	}
}

void App::Compare(const std::string &dbname)
{
	DatabaseMap::const_iterator tar[2];
	tar[0] = m_tars[0].find(dbname);
	tar[1] = m_tars[1].find(dbname);

	if( tar[0] == m_tars[0].end() || tar[1] == m_tars[1].end() )
		throw logic_error("Comparing non-existant database!" + dbname);

	Compare(dbname, tar[0]->second, tar[1]->second);
}

void App::Compare(const std::string &dbname,
			const DBDataList &one,
			const DBDataList &two)
{
	DBDataList::const_iterator
		b1 = one.begin(), e1 = one.end(),	// begin/end for one
		b2 = two.begin(), e2 = two.end(),	// begin/end for two
		s1, s2;					// search markers

	// if IDs are alike, compare
	// if not alike, then for each b1 and b2, do:
	//	search for id in opposite list
	//	if id found in opposite list, we're done, leave for next match
	//	if id not found, then entry has either been deleted or added
	//
	// NOTE: this algorithm assumes that both one and two are sorted!
	while( b1 != e1 || b2 != e2 ) {
		if( Alike(b1, b2, e1, e2 ) ) {
			Compare(*b1, *b2);
			++b1;
			++b2;
			continue;
		}
		else {
			// SearchCheck increments iterators if needed
			SearchCheck(b1, e1, two, "deleted");
			SearchCheck(b2, e2, one, "added");
		}
	}
}

void ChecksumDBData(const DBData &data, bool include_ids, std::string &sum)
{
	Barry::SHA_CTX m_ctx;

	SHA1_Init(&m_ctx);

	if( include_ids ) {
		SHA1_Update(&m_ctx, data.GetDBName().c_str(),
			data.GetDBName().size());

		uint8_t recType = data.GetRecType();
		SHA1_Update(&m_ctx, &recType, sizeof(recType));

		uint32_t uniqueId = data.GetUniqueId();
		SHA1_Update(&m_ctx, &uniqueId, sizeof(uniqueId));
	}

	int len = data.GetData().GetSize() - data.GetOffset();
	SHA1_Update(&m_ctx,
		data.GetData().GetData() + data.GetOffset(), len);

	unsigned char sha1[SHA_DIGEST_LENGTH];
	SHA1_Final(sha1, &m_ctx);

	ostringstream oss;
	for( int i = 0; i < SHA_DIGEST_LENGTH; i++ ) {
		oss << hex << setfill('0') << setw(2)
			<< (unsigned int) sha1[i];
	}
	sum = oss.str();
}

class CompareParser : public Barry::Parser
{
private:
//	const DBData &m_one, &m_two;

protected:
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	tname m_##tname; \

	ALL_KNOWN_PARSER_TYPES
//fixme;
public:
	static std::auto_ptr<CompareParser> Factory(const DBData &one,
					const DBData &two);
};

void App::Compare(const DBData &one, const DBData &two)
{
/*
	// make sure one and two are of the same database, or throw
	if( one.GetDBName() != two.GetDBName() )
		throw logic_error("Tried to compare records from different databases: " + one.GetDBName() + ", and " + two.GetDBName());

	// always compare the sums of the data first, and if match, done
	string sum1, sum2;
	ChecksumDBData(one, true, sum1);
	ChecksumDBData(two, true, sum2);
	if( sum1 == sum2 )
		return; // done

	// records are different, print concise report
	ShowDatabaseHeader(one.GetDBName());

	// if different, check if there's a parser available for this data
	// if not, display that these records differ, dump verbose if
	// needed, and done
	auto_ptr<CompareParser> parser = CompareParser::Factory(one, two);
	if( !parser.get() ) {
		ShowRecordDiff(one, two);
		return;
	}

	// if parser available, parser records and do a parsed record compare.
	// if alike, tell user that sums differ, but records are the same
	// if parsed records differ, tell user concisely, and if verbose
	// is turned on, then tell which records have changed.  If user always
	// wants a binary dump, then dump as well
	if( m_always_hex )
		ShowRecordDiff(one, two, parser.get());
	parser->ShowDifferingFields();
*/
}

/*
void App::ShowRecordDiff(const DBData &one, const DBData &two,
			CompareParser *parser)
{
	// if parser is null, print:
	//    UniqueID: sizes (one vs. two), X bytes differ
	//
	// otherwise, print:
	//    UniqueID: sizes (one vs. two), (custom display name)
	//

	// if verbose and parser is null, or if always_hex,
	// then display a (messy?) hex diff of the raw data
}
*/

bool App::Alike(DBDataList::const_iterator b1,
		DBDataList::const_iterator b2,
		DBDataList::const_iterator e1,
		DBDataList::const_iterator e2)
{
	if( b1 == e1 || b2 == e2 )
		return false;
	return b1->GetUniqueId() == b2->GetUniqueId();
}

void App::SearchCheck(DBDataList::const_iterator &b,
			DBDataList::const_iterator &e,
			const DBDataList &opposite_list,
			const std::string &action)
{
	// nothing to do if we're at end of list
	if( b == e )
		return;

	// if id is found in opposite list, we're done!
	// leave the iterator as-is for the next cycle's match
	if( IdExists(opposite_list, b->GetUniqueId()) )
		return;

	// id not found, so set return value
	m_main_return = 3;

	// if id not found, then entry has either been deleted or added
	// (action says which one), and we need to display the diff
	// and advance the iterator
	ShowDatabaseHeader(b->GetDBName());
	cout << "  0x" << hex << b->GetUniqueId() << ": record has been "
		<< action << " in " << "tar[1]" /*m_tarfiles[1]*/ << endl;
	if( m_verbose ) {
		DumpRecord(*b);
	}

	// advance!
	++b;
}

void App::DumpRecord(const DBData &data)
{
	// FIXME - dump record, either in hex, or in (condensed?) parsed format
}

void App::ShowDatabaseHeader(const std::string &dbname)
{
	if( dbname != m_last_dbname ) {
		m_last_dbname = dbname;
		cout << "In database: " << dbname << endl;

	}
}

int App::main(int argc, char *argv[])
{
	bool brief = false;
	string iconvCharset;

	// process command line options
	for(;;) {
		int cmd = getopt(argc, argv, "bd:D:hI:Sv");
		if( cmd == -1 )
			break;

		switch( cmd )
		{
		case 'b':	// use brief output
			brief = true;
			break;

		case 'd':	// database name to compare
			m_compare_list.push_back(optarg);
			break;

		case 'D':	// skip database to compare
			m_skip_list.push_back(optarg);
			break;

		case 'S':	// show parsers and builders
			ShowParsers();
			return 0;

		case 'I':	// international charset (iconv)
			iconvCharset = optarg;
			break;

		case 'v':	// verbose
			m_verbose = true;
			break;

		case 'h':	// help
		default:
			Usage();
			return 0;
		}
	}

	if( (optind + 2) > argc ) {
		Usage();
		return 0;
	}

	// save the tarball filenames for later processing
	// start out assuming both arguments are simple, no path filenames
	m_tarpaths[0] = m_tarfiles[0] = argv[optind];
	m_tarpaths[1] = m_tarfiles[1] = argv[optind+1];

	if( brief ) {
		// user wants brief markers... filenames must be huge! :-)
		m_tarfiles[0] = "tar[0]";
		m_tarfiles[1] = "tar[1]";
	}
	else {
		// attempt to trim paths to filenames only
		if( m_tarpaths[0].find('/') != string::npos )
			m_tarfiles[0] = m_tarpaths[0].substr(m_tarpaths[0].rfind('/') + 1);
		if( m_tarpaths[1].find('/') != string::npos )
			m_tarfiles[1] = m_tarpaths[1].substr(m_tarpaths[1].rfind('/') + 1);

		// double check... don't want both markers the same:
		if( m_tarfiles[0] == m_tarfiles[1] ) {
			// doh... back to where we started
			m_tarfiles[0] = m_tarpaths[0];
			m_tarfiles[1] = m_tarpaths[1];
		}
	}

	// display key for user
	cout << "tar[0] = " << m_tarpaths[0] << endl;
	cout << "tar[1] = " << m_tarpaths[1] << endl;

	// initialize the Barry library
	Barry::Init(m_verbose);

	// create an IConverter object if needed
	if( iconvCharset.size() ) {
		m_ic.reset( new IConverter(iconvCharset.c_str(), true) );
	}

	// load both tarballs into memory for easy comparisons
	LoadTarballs();

	// compare plain list of database names first
	CompareDatabaseNames();

	// compare the actual data
	CompareData();

	return m_main_return;
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

