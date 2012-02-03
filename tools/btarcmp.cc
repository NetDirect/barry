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
Still TODO: should have the ability to copy all differing records
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
   << "   -b        Use brief filename output\n"
   << "   -d db     Specify a specific database to compare.  Can be used\n"
   << "             multiple times.  If not used at all, all databases are\n"
   << "             compared.\n"
   << "   -D db     Specify a database name to skip.  If both -d and -D are\n"
   << "             used for the same database name, it will be skipped.\n"
   << "   -h        This help\n"
   << "   -I cs     International charset for string conversions\n"
   << "             Valid values here are available with 'iconv --list'\n"
   << "   -P        Only compare records that can be parsed\n"
   << "             This is the same as specifying -d for each database\n"
   << "             listed with -S.\n"
   << "   -S        Show list of supported database parsers\n"
   << "   -v        Show verbose diff output (twice to force hex output)\n"
   << "\n"
   << endl;
}


//////////////////////////////////////////////////////////////////////////////
// Utility functions and functors

bool DBDataCmp(const DBData &a, const DBData &b)
{
	return a.GetUniqueId() < b.GetUniqueId();
}

bool UnknownCmp(const UnknownField &a, const UnknownField &b)
{
	return a.type < b.type;
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


//////////////////////////////////////////////////////////////////////////////
// Parsed Compare class

class ParsedCompare
{
private:
	const DBData &m_one, &m_two;
	const IConverter *m_ic;
	bool m_known_record;
	std::string m_first_description;

public:
	ParsedCompare(const DBData &one, const DBData &two,
		const IConverter *ic = 0);

	bool CanParse() const { return m_known_record; }
	const std::string& GetDescription() const { return m_first_description;}

	/// Returns true if differing fields found and displayed.
	/// False if no differences found.
	bool ShowDifferingFields();
};

ParsedCompare::ParsedCompare(const DBData &one,
				const DBData &two,
				const IConverter *ic)
	: m_one(one)
	, m_two(two)
	, m_ic(ic)
	, m_known_record(false)
{
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	else if( tname::GetDBName() == one.GetDBName() ) { \
		m_known_record = true; \
		tname a; \
		ParseDBData(m_one, a, m_ic); \
		m_first_description = a.GetDescription(); \
	}

	if( one.GetDBName() != two.GetDBName() ) {
		throw logic_error("Different database types in ParsedCompare ctor!");
	}
	// fall through and use the else's
	ALL_KNOWN_PARSER_TYPES
}

template <class RecordT>
class FieldHandler
{
private:
	const RecordT &m_one, &m_two;
	mutable bool m_found_difference;

public:
	FieldHandler(const RecordT &one, const RecordT &two)
		: m_one(one)
		, m_two(two)
		, m_found_difference(false)
	{
	}

	bool Differing() const { return m_found_difference; }

	void operator()(EnumFieldBase<RecordT> *ep,
		const FieldIdentity &id) const
	{
		if( ep->GetValue(m_one) == ep->GetValue(m_two) )
			return;

		m_found_difference = true;
		cout << "   " << id.Name << ":\n"
			<< "         tar[0] = "
			<< ep->GetName(ep->GetValue(m_one))
			<< " (" << ep->GetValue(m_one) << ")\n"
			<< "         tar[1] = "
			<< ep->GetName(ep->GetValue(m_two))
			<< " (" << ep->GetValue(m_two) << ")"
			<< endl;
	}

	void operator()(typename FieldHandle<RecordT>::PostalPointer pp,
		const FieldIdentity &id) const
	{
		const std::string
			&a = m_one.*(pp.m_PostalAddress).*(pp.m_PostalField),
			&b = m_two.*(pp.m_PostalAddress).*(pp.m_PostalField);

		if( a == b )
			return;

		m_found_difference = true;
		cout << "   " << id.Name << ":\n"
			<< "         tar[0] = '" << a << "'\n"
			<< "         tar[1] = '" << b << "'"
			<< endl;
	}

	void operator()(std::string RecordT::* mp, const FieldIdentity &id) const
	{
		if( m_one.*mp == m_two.*mp )
			return;

		m_found_difference = true;
		cout << "   " << id.Name << ":\n"
			<< "         tar[0] = '"
			<< Cr2LfWrapper(m_one.*mp) << "'\n"
			<< "         tar[1] = '"
			<< Cr2LfWrapper(m_two.*mp) << "'"
			<< endl;
	}

	void operator()(UnknownsType RecordT::* mp, const FieldIdentity &id) const
	{
		UnknownsType a = m_one.*mp, b = m_two.*mp;

		sort(a.begin(), a.end(), UnknownCmp);
		sort(b.begin(), b.end(), UnknownCmp);

		if( a == b )
			return;

		m_found_difference = true;
		cout << "   " << id.Name << ":\n"
			<< "         tar[0] = '" << a << "'\n"
			<< "         tar[1] = '" << b << "'"
			<< endl;
	}

	template <class TypeT>
	void operator()(TypeT RecordT::* mp, const FieldIdentity &id) const
	{
		if( m_one.*mp == m_two.*mp )
			return;

		m_found_difference = true;
		cout << "   " << id.Name << ":\n"
			<< "         tar[0] = '" << m_one.*mp << "'\n"
			<< "         tar[1] = '" << m_two.*mp << "'"
			<< endl;
	}
};

template <class RecordT>
bool DoParsedCompare(const RecordT &a, const RecordT &b)
{
	FieldHandler<RecordT> handler(a, b);
	ForEachField(RecordT::GetFieldHandles(), handler);
	return handler.Differing();
}

/// Returns true if differing fields found and displayed.
/// False if no differences found.
bool ParsedCompare::ShowDifferingFields()
{
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	else if( tname::GetDBName() == m_one.GetDBName() ) { \
		tname a, b; \
		ParseDBData(m_one, a, m_ic); \
		ParseDBData(m_two, b, m_ic); \
		return DoParsedCompare<tname>(a, b); \
	}

	if( !m_known_record ) {
		return false;
	}

	ALL_KNOWN_PARSER_TYPES

	else {
		return false;
	}
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

	int m_main_return;		// 0 - success
					// 1 - low level error or logic error
					// 2 - databases lists not the same
					// 3 - a record was added or deleted
	bool m_verbose;
	bool m_always_hex;
	bool m_sort_on_load;		// if true, sort each database by
					// Unique ID after loading from tarball
	bool m_include_ids;		// if true, include DBData IDs in SHA1

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

	void ShowRecordDiff(const DBData &one, const DBData &two,
		ParsedCompare &pc);
	void DumpRecord(const DBData &data);
	void ShowDatabaseHeader(const std::string &dbname);
	void AddParsersToCompare();

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
// Misc helpers dependent on App

bool IdExists(const App::DBDataList &list, uint32_t id)
{
	return find_if(list.begin(), list.end(), DBDataIdCmp(id)) != list.end();
}

//////////////////////////////////////////////////////////////////////////////
// Member function definitions

App::App()
	: m_main_return(0)
	, m_verbose(false)
	, m_always_hex(false)
	, m_sort_on_load(true)
	, m_include_ids(true)
{
}

void App::ShowParsers()
{
	cout << "Supported Database parsers:\n";

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	{ \
		cout << "   " << tname::GetDBName() << "\n      "; \
		FieldHandle<tname>::ListT::const_iterator \
				fhi = tname::GetFieldHandles().begin(), \
				fhe = tname::GetFieldHandles().end(); \
		for( int count = 0, len = 6; fhi != fhe; ++fhi, ++count ) { \
			if( count ) { \
				cout << ", "; \
				len += 2; \
			} \
			std::string name = fhi->GetIdentity().Name; \
			if( len + name.size() >= 75 ) { \
				cout << "\n      "; \
				len = 6; \
			} \
			cout << name; \
			len += name.size(); \
		} \
		cout << "\n"; \
	}

	ALL_KNOWN_PARSER_TYPES

	cout << endl;
}

void App::AddParsersToCompare()
{
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	m_compare_list.push_back(tname::GetDBName());

	ALL_KNOWN_PARSER_TYPES
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
			if( m_sort_on_load )
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

void App::Compare(const DBData &one, const DBData &two)
{
	// make sure one and two are of the same database, or throw
	if( one.GetDBName() != two.GetDBName() )
		throw logic_error("Tried to compare records from different databases: " + one.GetDBName() + ", and " + two.GetDBName());

	// always compare the sums of the data first, and if match, done
	string sum1, sum2;
	ChecksumDBData(one, m_include_ids, sum1);
	ChecksumDBData(two, m_include_ids, sum2);
	if( sum1 == sum2 )
		return; // done

	// records are different, print concise report
	ShowDatabaseHeader(one.GetDBName());

	// if different, check if there's a parser available for this data
	// if not, display that these records differ, dump verbose if
	// needed, and done
	ParsedCompare pc(one, two, m_ic.get());
	ShowRecordDiff(one, two, pc);
}

void App::ShowRecordDiff(const DBData &one,
			const DBData &two,
			ParsedCompare &pc)
{
	if( !pc.CanParse() ) {
		// if can't parse, print:
		//    UniqueID: sizes (one vs. two), X bytes differ
		// then the differing fields
		cout << "  0x" << hex << one.GetUniqueId() << ": differs: "
			<< dec
			<< "sizes (" << one.GetData().GetSize()
			<< " vs. " << two.GetData().GetSize()
			<< "), SHA1 sums differ"
			<< endl;
	}
	else {
		// otherwise, print:
		//    UniqueID: sizes (one vs. two), (custom display name)
		cout << "  0x" << hex << one.GetUniqueId() << ": differs: "
			<< dec
			<< "sizes (" << one.GetData().GetSize()
			<< " vs. " << two.GetData().GetSize()
			<< "), "
			<< pc.GetDescription()
			<< endl;

		if( !pc.ShowDifferingFields() ) {
			// no difference found...
			cout << "No differences found in parsed records, but SHA1 sums differ." << endl;
		}
	}

	// if verbose and parser is null, or if always_hex,
	// then display a (messy?) hex diff of the raw data
	if( (m_verbose && !pc.CanParse()) || m_always_hex ) {
		cout << "   Hex diff of record:" << endl;
		cout << Diff(one.GetData(), two.GetData()) << endl;
	}
}

bool App::Alike(DBDataList::const_iterator b1,
		DBDataList::const_iterator b2,
		DBDataList::const_iterator e1,
		DBDataList::const_iterator e2)
{
	if( b1 == e1 || b2 == e2 )
		return false;
	return b1->GetUniqueId() == b2->GetUniqueId();
}

std::string GetDBDescription(const DBData &data, const IConverter *ic)
{
	string desc;

	// try to parse it
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	if( data.GetDBName() == tname::GetDBName() ) { \
		tname rec; \
		ParseDBData(data, rec, ic); \
		return rec.GetDescription(); \
	}

	ALL_KNOWN_PARSER_TYPES

	return desc;
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
		<< action << " in " << "tar[1]";
	string desc = GetDBDescription(*b, m_ic.get());
	if( desc.size() ) {
		cout << ": " << desc << endl;
	}
	else {
		cout << endl;
	}
	if( m_verbose ) {
		DumpRecord(*b);
	}

	// advance!
	++b;
}

void App::DumpRecord(const DBData &data)
{
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	if( data.GetDBName() == tname::GetDBName() ) { \
		tname rec; \
		ParseDBData(data, rec, m_ic.get()); \
		cout << rec << endl; \
		return; \
	}

	ALL_KNOWN_PARSER_TYPES

	// if we get here, it's not a known record, so just dump the hex
	cout << data.GetData() << endl;
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
		int cmd = getopt(argc, argv, "bd:D:hI:PSv");
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

		case 'P':	// only compare parseable records
			AddParsersToCompare();
			break;

		case 'S':	// show parsers and builders
			ShowParsers();
			return 0;

		case 'I':	// international charset (iconv)
			iconvCharset = optarg;
			break;

		case 'v':	// verbose
			if( !m_verbose )
				m_verbose = true;
			else
				m_always_hex = true;
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
	Barry::Init(false);

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

