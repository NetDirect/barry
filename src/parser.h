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

// forward declarations
namespace Barry {
	class IConverter;
}

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
class RecordParser : public Parser
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
};

} // namespace Barry

#endif

