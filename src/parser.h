///
/// \file	parser.h
///		Virtual parser wrapper
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

#ifndef __BARRY_PARSER_H__
#define __BARRY_PARSER_H__

#include "data.h"
#include "protocol.h"
#include "debug.h"

namespace Barry {

// also acts as a null parser
//
// Parser class
//
/// Base class for the parser functor hierarchy.  If in debug mode, this
/// class can be used as a null parser.  Call Init() and the protocol
/// will be dumped to stdout and no parsing will be done.
///
class Parser
{
public:
	Parser() {}
	virtual ~Parser() {}

	virtual bool operator()(const Data &data, size_t offset) { return true; }

	virtual bool GetOperation(const Data &data, unsigned int &operation);
};


//
// RecordParser template class
//
/// Template class for easy creation of specific parser objects.  This template
/// takes the following template arguments:
///
///	- Record: One of the record parser classes in record.h
///	- Storage: A custom storage functor class.  An object of this type
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
template <class Record, class Storage>
class RecordParser : public Parser
{
	Storage *m_store;
	bool m_owned;

public:
	/// Constructor that references an externally managed storage object.
	RecordParser(Storage &storage)
		: m_store(&storage), m_owned(false) {}

	/// Constructor that references a locally managed storage object.
	/// The pointer passed in will be stored, and freed when this class
	/// is destroyed.  It is safe to call this constructor with
	/// a 'new'ly created storage object.
	RecordParser(Storage *storage)
		: m_store(storage), m_owned(true) {}

	~RecordParser()
	{
		if( this->m_owned )
			delete m_store;
	}

	virtual bool CheckHeaderSize(const Data &data, size_t offset, unsigned int operation)
	{
		size_t recordsize;
		switch( operation )
		{
		case SB_DBOP_GET_RECORDS:
			// using the new protocol
			recordsize = Record::GetProtocolRecordSize();
			break;

		case SB_DBOP_OLD_GET_RECORDS_REPLY:
			// using the old protocol
			recordsize = Record::GetOldProtocolRecordSize();
			break;

		default:
			// unknown protocol
			dout("Unknown protocol");
			return false;
		}

		// return true if header is ok
		return data.GetSize() > (offset + recordsize);
	}

	/// Functor member called by Controller::LoadDatabase() during
	/// processing.
	virtual bool operator()(const Data &data, size_t offset)
	{
		unsigned int operation;
		if( !GetOperation(data, operation) )
			return false;
		if( !CheckHeaderSize(data, offset, operation) )
			return false;

		Record rec;
		rec.Parse(data, offset, operation);
		(*m_store)(rec);
		return true;
	}
};

} // namespace Barry

#endif

