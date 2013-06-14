///
/// \file	parser.cc
///		Virtual parser wrapper
///

/*
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "i18n.h"
#include "parser.h"
#include "r_calendar.h"
#include "r_calllog.h"
#include "r_bookmark.h"
#include "r_contact.h"
#include "r_memo.h"
#include "r_message.h"
#include "r_servicebook.h"
#include "r_task.h"
#include "r_pin_message.h"
#include "r_saved_message.h"
#include "r_sms.h"
#include "r_folder.h"
#include "r_timezone.h"
#include "r_cstore.h"
#include "r_hhagent.h"
#include "ios_state.h"

#include <iostream>
#include <memory>

using namespace std;

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// HexDumpParser class

HexDumpParser::HexDumpParser(std::ostream &os)
	: m_os(os)
{
}

void HexDumpParser::ParseRecord(const Barry::DBData &data,
				const IConverter *ic)
{
	ios_format_state state(m_os);

	if( m_last_dbname != data.GetDBName() ) {
		m_os << _("Records for database: ") << data.GetDBName() << endl;
		m_last_dbname = data.GetDBName();
	}

	m_os << _("Raw record dump for record: ") << "0x"
		<< hex << data.GetUniqueId()
		<< ", " << _("type: ")
		<< "0x" << hex << (unsigned int) data.GetRecType()
		<< ", " << _("offset: ")
		<< "0x" << hex << data.GetOffset()
		<< endl;
	m_os << data.GetData() << endl;
}


//////////////////////////////////////////////////////////////////////////////
// DBNamesOnlyParser class

DBNamesOnlyParser::DBNamesOnlyParser(std::ostream &os)
	: m_os(os)
{
}

void DBNamesOnlyParser::ParseRecord(const Barry::DBData &data,
				const IConverter *ic)
{
	ios_format_state state(m_os);

	if( m_last_dbname != data.GetDBName() ) {
		m_os << data.GetDBName() << endl;
		m_last_dbname = data.GetDBName();
	}
}


//////////////////////////////////////////////////////////////////////////////
// MultiRecordParser class

// takes ownership of default_parser!
MultiRecordParser::MultiRecordParser(Parser *default_parser)
	: m_delete_default(default_parser) // takes ownership
	, m_default(default_parser)
{
}

// does not take ownership of the default_parser
MultiRecordParser::MultiRecordParser(Parser &default_parser)
	: m_delete_default(0)	// no ownership of reference
	, m_default(&default_parser)
{
}

MultiRecordParser::~MultiRecordParser()
{
	map_type::iterator i = m_parsers.begin();
	for( ; i != m_parsers.end(); ++i ) {
		delete i->second;
	}

	// and the default parser
	delete m_delete_default;
}

void MultiRecordParser::Add(const std::string &dbname, Parser *parser)
{
	std::auto_ptr<Parser> p(parser);

	map_type::iterator i = m_parsers.find(dbname);
	if( i != m_parsers.end() ) {
		// found existing parser, so delete it first
		delete i->second;

		// assign it
		i->second = p.release();
	}
	else {
		m_parsers[dbname] = p.get();
		p.release();
	}
}

// takes ownership of parser!
void MultiRecordParser::Add(RecordParserBase *parser)
{
	std::auto_ptr<Parser> p(parser);
	std::string name = parser->GetDBName();
	Add(name, p.release());
}

bool MultiRecordParser::Add(const std::string &dbname,
				std::ostream &os)
{
	std::auto_ptr<Parser> p;

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) if( dbname == tname::GetDBName() ) { p.reset( new RecordParser<tname, DumpStore<tname> > (new DumpStore<tname>(os)) ); }

	// check for recognized database names
	ALL_KNOWN_PARSER_TYPES

	if( !p.get() ) {
		// name not known
		return false;
	}

	Add(dbname, p.release());
	return true;
}

bool MultiRecordParser::Add(const std::string &dbname, AllRecordStore &store)
{
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	if( dbname == tname::GetDBName() ) { \
		Add(dbname, new RecordParser<tname, AllRecordStore>(store)); \
		return true; \
	}

	// check for recognized database names
	ALL_KNOWN_PARSER_TYPES

	// if we get here, record was not found
	return false;
}

// Parser overrides
void MultiRecordParser::ParseRecord(const DBData &data, const IConverter *ic)
{
	// search for a named parser
	map_type::iterator i = m_parsers.find(data.GetDBName());
	if( i != m_parsers.end() ) {
		// found one, use it
		i->second->ParseRecord(data, ic);
	}
	else if( m_default ) {
		// use default parser
		m_default->ParseRecord(data, ic);
	}
}


//////////////////////////////////////////////////////////////////////////////
// AllRecordDumpStore class

// Use the macro here to implement the overrides, so that
// the compiler will catch if we are missing any.
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	void AllRecordDumpStore::operator() (const Barry::tname &r) \
	{ \
		m_os << r << std::endl; \
	}

ALL_KNOWN_PARSER_TYPES


//////////////////////////////////////////////////////////////////////////////
// AllRecordDumpParser class

AllRecordParser::AllRecordParser(std::ostream &os,
				Parser *default_parser,
				AllRecordStore *store)
	: MultiRecordParser(default_parser)
	, m_store(store)	// takes ownership here
{
	AddRecords(&os, store);
}

// does not take ownership of default_parser or store
AllRecordParser::AllRecordParser(Parser &default_parser, AllRecordStore &store)
	: MultiRecordParser(default_parser)
	, m_store(0)
{
	AddRecords(0, &store);
}

AllRecordParser::~AllRecordParser()
{
	delete m_store;
}

void AllRecordParser::AddRecords(std::ostream *os, AllRecordStore *store)
{
	// Does not allow RecordParser<> to own store, since we're using
	// it multiple times as the same store for each record type.
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	if( store ) { \
		Add( new RecordParser<tname, AllRecordStore>(*store)); \
	} else if( os ) { \
		Add(tname::GetDBName(), *os); \
	}

	ALL_KNOWN_PARSER_TYPES;
}


//////////////////////////////////////////////////////////////////////////////
// TeeParser class

TeeParser::TeeParser()
{
}

TeeParser::~TeeParser()
{
	// free all the owned parser pointers
	for( parser_list_type::iterator i = m_owned_parsers.begin();
		i != m_owned_parsers.end();
		++i )
	{
		delete *i;
	}
}

// takes ownership of the pointer!
void TeeParser::Add(Parser *p)
{
	std::auto_ptr<Parser> ap(p);
	m_owned_parsers.push_back(ap.get());
	ap.release();
}

// does NOT take ownership
void TeeParser::Add(Parser &p)
{
	m_external_parsers.push_back(&p);
}

void TeeParser::ParseRecord(const DBData &data, const IConverter *ic)
{
	// call all owned parsers
	for( parser_list_type::iterator i = m_owned_parsers.begin();
		i != m_owned_parsers.end();
		++i )
	{
		(*i)->ParseRecord(data, ic);
	}

	// call all external parsers
	for( parser_list_type::iterator i = m_external_parsers.begin();
		i != m_external_parsers.end();
		++i )
	{
		(*i)->ParseRecord(data, ic);
	}
}

} // namespace Barry

