///
/// \file	parser.h
///		Virtual parser wrapper
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_PARSER_H__
#define __BARRY_PARSER_H__

#include "dll.h"
#include "data.h"
#include "protocol.h"
#include <stdint.h>		// for uint32_t
#include <iosfwd>
#include <map>

// forward declarations
namespace Barry {
	class IConverter;
	class Contact;
	class Message;
	class Calendar;
	class CalendarAll;
	class CallLog;
	class Bookmark;
	class ServiceBook;
	class Memo;
	class Task;
	class PINMessage;
	class SavedMessage;
	class Sms;
	class Folder;
	class Timezone;
}

//
// This macro can be used to automatically generate code for all known
// record types.  Just #undef HANDLE_PARSER, then #define it to whatever
// you need, then use ALL_KNOWN_PARSER_TYPES.  See parser.cc for
// various examples.
//
// These are sorted so their GetDBName()'s will display in alphabetical order.
//
#define ALL_KNOWN_PARSER_TYPES \
	HANDLE_PARSER(Contact) \
	HANDLE_PARSER(Bookmark) \
	HANDLE_PARSER(Calendar) \
	HANDLE_PARSER(CalendarAll) \
	HANDLE_PARSER(Folder) \
	HANDLE_PARSER(Memo) \
	HANDLE_PARSER(Message) \
	HANDLE_PARSER(CallLog) \
	HANDLE_PARSER(PINMessage) \
	HANDLE_PARSER(SavedMessage) \
	HANDLE_PARSER(ServiceBook) \
	HANDLE_PARSER(Sms) \
	HANDLE_PARSER(Task) \
	HANDLE_PARSER(Timezone)

namespace Barry {

//
// Parser class
//
/// Base class for the parser hierarchy.
///
/// This class provides the interface that the Controller class uses
/// to pass raw data it reads from the device.  The Controller, along
/// with the Packet class, calls each of the virtual functions below
/// in the same order.
///
/// This class is kept as a pure abstract class, in order to make sure
/// that the compiler will catch any API changes, for code derived
/// from it.
///
class BXEXPORT Parser
{
public:
	Parser() {}
	virtual ~Parser() {}

	/// Called to parse sub fields in the raw data packet.
	virtual void ParseRecord(const DBData &data, const IConverter *ic) = 0;
};


//
// NullParser class
//
/// If in debug mode, this class can be used as a null parser.
/// Call Init() and the protocol will be dumped to stdout and
/// no parsing will be done.
///
/// Do NOT derive your own personal parser classes from this,
/// unless you are perfectly confident that you will catch
/// future API changes on the devel tree without the compiler's
/// help.
///
class BXEXPORT NullParser : public Parser
{
public:
	NullParser() {}
	virtual ~NullParser() {}

	virtual void ParseRecord(const DBData &data, const IConverter *ic) {}
};

//
// HexDumpParser
//
/// Dumps raw hex of the given DBData to the given stream.
///
/// Do NOT derive your own personal parser classes from this,
/// unless you are perfectly confident that you will catch
/// future API changes on the devel tree without the compiler's
/// help.
///
class BXEXPORT HexDumpParser : public Parser
{
	std::ostream &m_os;

public:
	explicit HexDumpParser(std::ostream &os);

	virtual void ParseRecord(const Barry::DBData &data,
				 const IConverter *ic);
};

//
// RecordParserBase
//
/// Abstract base class for the following RecordParser template, that exposes
/// some information on the specifics that the record parser can handle.
/// Specifically, it exposes the database name it is able to parse
///
class BXEXPORT RecordParserBase : public Parser
{
public:
	// These functions are always valid, regardless of the
	// state of the parser.
	virtual const char * GetDBName() const = 0;
	virtual uint8_t GetDefaultRecType() const = 0;

	// These functions depend on the parser having just parsed
	// a record successfully.
	virtual bool IsRecordValid() const = 0;
	virtual uint8_t GetRecType() const = 0;
	virtual uint32_t GetUniqueId() const = 0;
	virtual void Dump(std::ostream &os) const = 0;
};


//
// Note: Store classes take parsed Record objects as a functor.
//       Parser classes deal with raw data, while Store classes deal with
//       parsed Record objects.
//

//
// NullStore
//
/// A Storage class for RecordParser<> that does nothing, for the cases
/// where you only want to dump parsed record data to a stream.
///
template <class RecordT>
class NullStore
{
public:
	void operator() (const RecordT &r)
	{
	}
};

//
// DumpStore
//
/// A Storage class for RecordParser<> that dumps the parsed record data
/// to the given stream.
///
template <class RecordT>
class DumpStore
{
	std::ostream &m_os;

public:
	explicit DumpStore(std::ostream &os)
		: m_os(os)
	{
	}

	void operator() (const RecordT &r)
	{
		r.Dump(m_os);
	}
};

//
// RecordParser template class
//
/// Template class for easy creation of specific parser objects.  This template
/// takes the following template arguments:
///
///	- RecordT: One of the record parser classes in record.h
///	- StorageT: A custom storage functor class.  An object of this type
///		will be called as a function with parsed Record as an
///		argument.  This happens on the fly as the data is retrieved
///		from the device over USB, so it should not block forever.
///
/// Example LoadDatabase() call:
///
/// <pre>
/// struct StoreContact
/// {
///     std::vector<Contact> &amp;array;
///     StoreContact(std::vector<Contact> &amp;a) : array(a) {}
///     void operator() (const Contact &amp;c)
///     {
///         array.push_back(c);
///     }
/// };
///
/// Controller con(probeResult);
/// con.OpenMode(Controller::Desktop);
/// std::vector<Contact> contactList;
/// StoreContact storage(contactList);
/// RecordParser<Contact, StoreContact> parser(storage);
/// con.LoadDatabase(con.GetDBID("Address Book"), parser);
/// </pre>
///
template <class RecordT, class StorageT>
class RecordParser : public RecordParserBase
{
	StorageT *m_store;
	bool m_owned;
	RecordT m_rec;
	bool m_record_valid;

public:
	/// Constructor that references an externally managed storage object.
	RecordParser(StorageT &storage)
		: m_store(&storage)
		, m_owned(false)
		, m_record_valid(false)
	{
	}

	/// Constructor that references a locally managed storage object.
	/// The pointer passed in will be stored, and freed when this class
	/// is destroyed.  It is safe to call this constructor with
	/// a 'new'ly created storage object.
	RecordParser(StorageT *storage = 0)
		: m_store(storage)
		, m_owned(true)
		, m_record_valid(false)
	{
	}

	~RecordParser()
	{
		if( this->m_owned )
			delete m_store;
	}

	virtual StorageT* GetStore()
	{
		return m_store;
	}

	virtual const StorageT* GetStore() const
	{
		return m_store;
	}

	virtual void ParseRecord(const DBData &data, const IConverter *ic)
	{
		m_rec = RecordT();
		m_record_valid = false;

		m_rec.SetIds(data.GetRecType(), data.GetUniqueId());
		size_t offset = data.GetOffset();
		m_rec.ParseHeader(data.GetData(), offset);
		m_rec.ParseFields(data.GetData(), offset, ic);
		m_record_valid = true;

		if( m_store )
			(*m_store)(m_rec);
	}

	//
	// RecordParserBase overrides
	//

	// These functions are always valid, regardless of the
	// state of the parser.
	virtual const char * GetDBName() const
	{
		return RecordT::GetDBName();
	}

	virtual uint8_t GetDefaultRecType() const
	{
		return RecordT::GetDefaultRecType();
	}

	// These functions depend on the parser having just parsed
	// a record successfully.
	virtual bool IsRecordValid() const
	{
		return m_record_valid;
	}

	virtual uint8_t GetRecType() const
	{
		return m_rec.GetRecType();
	}

	virtual uint32_t GetUniqueId() const
	{
		return m_rec.GetUniqueId();
	}

	virtual void Dump(std::ostream &os) const
	{
		m_rec.Dump(os);
	}
};

//
// AllRecordStore
//
/// Base class with overloaded functor behaviour for all available
/// record classes.  To be used with AllRecordParser.
///
class BXEXPORT AllRecordStore
{
public:
	AllRecordStore() {}
	virtual ~AllRecordStore() {}

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	virtual void operator() (const Barry::tname &) = 0;

	ALL_KNOWN_PARSER_TYPES
};

//
// MultiRecordParser
//
/// Container parser class that accepts multiple Parser objects
/// (often RecordParser<> objects but they don't have to be) and
/// automatically routes incoming records to the appropriate parser.
/// Note that this container owns *all* Parser objects, and will
/// free them upon destruction.
///
/// Incoming records that have no matching parser are passed to the
/// default parser object, if one exists, otherwise they are dropped
/// silently.  The default parser object is also owned by the container,
/// and will be freed on destruction.
///
/// Do NOT derive your own personal parser classes from this,
/// unless you are perfectly confident that you will catch
/// future API changes on the devel tree without the compiler's
/// help.
///
class BXEXPORT MultiRecordParser : public Parser
{
	typedef std::map<std::string, Parser*>			map_type;

	Parser *m_default;
	map_type m_parsers;

public:
	// takes ownership of default_parser!
	explicit MultiRecordParser(Parser *default_parser = 0);
	~MultiRecordParser();

	/// Adds given parser to list and takes ownership of it
	void Add(const std::string &dbname, Parser *parser);

	/// Adds given parser to list and takes ownership of it
	void Add(RecordParserBase *parser);

	/// Creates a RecordParser<> object using the given record
	/// type and AllRecordStore.  Does NOT take ownership of the
	/// store object, since it can be used multiple times for
	/// multiple records.
	template <class RecordT>
	void Add(AllRecordStore &store)
	{
		Add( RecordT::GetDBName(),
			new RecordParser<RecordT, AllRecordStore>(store) );
	}

	/// Two helper template functions that create the RecordParser<>
	/// automatically based on the function call.  Both pointer and
	/// reference versions.
	template <class RecordT, class StorageT>
	void Add(StorageT *store)
	{
		Add( RecordT::GetDBName(),
			new RecordParser<RecordT, StorageT>(store) );
	}

	template <class RecordT, class StorageT>
	void Add(StorageT &store)
	{
		Add( RecordT::GetDBName(),
			new RecordParser<RecordT, StorageT>(store) );
	}

	/// Creates a RecordParser<> object for the given database name,
	/// using DumpStore<> with the given stream for the output,
	/// and adds it to list.
	/// Returns false if there is no known Record class for dbname.
	bool Add(const std::string &dbname, std::ostream &os);

	/// Creates a RecordParser<> object for the given database name,
	/// using the given store object.
	/// Returns false if there is no known Record class for dbname.
	bool Add(const std::string &dbname, AllRecordStore &store);

	// Parser overrides
	virtual void ParseRecord(const DBData &data, const IConverter *ic);
};

//
// AllRecordDumpStore
//
/// Derived from AllRecordStore, which just calls each record's
/// Dump() member with the given stream.
///
class BXEXPORT AllRecordDumpStore : public AllRecordStore
{
protected:
	std::ostream &m_os;

public:
	explicit AllRecordDumpStore(std::ostream &os)
		: m_os(os)
	{
	}

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	virtual void operator() (const Barry::tname &);

	ALL_KNOWN_PARSER_TYPES
};

//
// AllRecordParser
//
/// Convenience parser that creates a MultiRecordParser with all known
/// record parsers added.  If an AllRecordStore pointer is passed in,
/// this class takes ownership of it, and uses it as the store object
/// for all the RecordParser<> objects it creates.  If not, then
/// a custom DumpStore<> object is created with the given stream
/// for each RecordParser<> added.
///
/// The default parser object behaves just like MultiRecordParser
///
/// This class takes ownership of all pointers passed in.
///
class BXEXPORT AllRecordParser : public MultiRecordParser
{
	AllRecordStore *m_store;

public:
	// takes ownership of default_parser and store!
	explicit AllRecordParser(std::ostream &os,
		Parser *default_parser = 0,
		AllRecordStore *store = 0);
	~AllRecordParser();
};

//
// TeeParser
//
/// Sends incoming DBData objects to all the parsers in its list.
/// This parser container does NOT own the parsers added.
///
class BXEXPORT TeeParser : public Parser
{
	typedef std::vector<Parser*>			parser_list_type;

	parser_list_type m_external_parsers, m_owned_parsers;

public:
	TeeParser();
	~TeeParser();

	/// Adds parser to internal list, and takes ownership of the
	/// pointer.
	void Add(Parser *p);

	/// Adds parser to internal list.  Does NOT own the parser reference.
	void Add(Parser &p);

	void ParseRecord(const DBData &data, const IConverter *ic);
};

} // namespace Barry

#endif

