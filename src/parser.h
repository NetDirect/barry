///
/// \file	parser.h
///		Virtual parser wrapper
///

/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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
namespace Barry { class Data; }

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

	/// Reset and prepare for a new raw data packet
	virtual void Clear() = 0;

	/// Stores the IDs
	virtual void SetIds(uint8_t RecType, uint32_t UniqueId) = 0;

	/// Called to parse the header portion of the raw data packet.
	/// data contains the entire packet, and offset contains the
	/// location at which to start parsing.
	virtual void ParseHeader(const Data &data, size_t &offset) = 0;

	/// Called to parse sub fields in the raw data packet.
	/// The same data is passed as was passed in ParseHeader,
	/// only the offset will be updated if it was advanced during
	/// the header parsing.
	virtual void ParseFields(const Data &data, size_t &offset) = 0;

	/// Called at the very end of record parsing, and used to
	/// store the final packet somewhere, either in memory, disk, etc.
	virtual void Store() = 0;
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

	/// Reset and prepare for a new raw data packet
	virtual void Clear() {}

	/// Stores the IDs
	virtual void SetIds(uint8_t RecType, uint32_t UniqueId) {}

	/// Called to parse the header portion of the raw data packet.
	/// data contains the entire packet, and offset contains the
	/// location at which to start parsing.
	virtual void ParseHeader(const Data &data, size_t &offset) {}

	/// Called to parse sub fields in the raw data packet.
	/// The same data is passed as was passed in ParseHeader,
	/// only the offset will be updated if it was advanced during
	/// the header parsing.
	virtual void ParseFields(const Data &data, size_t &offset) {}

	/// Called at the very end of record parsing, and used to
	/// store the final packet somewhere, either in memory, disk, etc.
	virtual void Store() {}
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

public:
	/// Constructor that references an externally managed storage object.
	RecordParser(StorageT &storage)
		: m_store(&storage), m_owned(false) {}

	/// Constructor that references a locally managed storage object.
	/// The pointer passed in will be stored, and freed when this class
	/// is destroyed.  It is safe to call this constructor with
	/// a 'new'ly created storage object.
	RecordParser(StorageT *storage)
		: m_store(storage), m_owned(true) {}

	~RecordParser()
	{
		if( this->m_owned )
			delete m_store;
	}

	virtual void Clear()
	{
		m_rec = RecordT();
	}

	virtual void SetIds(uint8_t RecType, uint32_t UniqueId)
	{
		m_rec.SetIds(RecType, UniqueId);
	}

	virtual void ParseHeader(const Data &data, size_t &offset)
	{
		m_rec.ParseHeader(data, offset);
	}

	virtual void ParseFields(const Data &data, size_t &offset)
	{
		m_rec.ParseFields(data, offset);
	}

	virtual void Store()
	{
		(*m_store)(m_rec);
	}
};

} // namespace Barry

#endif

