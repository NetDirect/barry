///
/// \file	builder.h
///		Virtual protocol packet builder wrapper
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

#ifndef __BARRY_BUILDER_H__
#define __BARRY_BUILDER_H__

//#include "data.h"
//#include "protocol.h"
//#include "debug.h"

namespace Barry {

//
// Builder class
//
/// Base class for the builder functor hierarchy.
///
class Builder
{
public:
	Builder() {}
	virtual ~Builder() {}

	virtual bool operator()(Data &data, unsigned int databaseId) = 0;
};


//
// RecordBuilder template class
//
/// Template class for easy creation of specific protocol packet builder
/// objects.  This template takes the following template arguments:
///
///	- Record: One of the record classes in record.h
///	- Storage: A custom storage functor class.  An object of this type
///		will be called as a function with empty Record as an
///		argument.  This happens on the fly as the data is sent
///		to the device over USB, so it should not block forever.
///
/// Example SaveDatabase() call:
///
/// <pre>
fixme - document this
/// struct StoreContact
/// {
///     const std::vector<Contact> &amp;array;
///     std::vector<Contact>::const_iterator ci;
///     StoreContact(const std::vector<Contact> &amp;a)
///         : array(a), ci(array.begin()) {}
///     bool operator() (Contact &amp;c)
///     {
///         if( ci != array.end() ) {
///             c = *ci;
///             ci++;
///             return true;
///         }
///         return false;
///     }
/// };
///
/// Controller con(probeResult);
/// con.OpenMode(Controller::Desktop);
/// std::vector<Contact> contactList;
/// StoreContact storage(contactList);
/// RecordBuilder<Contact, StoreContact> builder(storage);
/// con.SaveDatabase(con.GetDBID("Address Book"), builder);
/// </pre>
///
template <class Record, class Storage>
class RecordBuilder : public Builder
{
	Storage *m_storage;
	bool m_owned;

public:
	/// Constructor that references an externally managed storage object.
	RecordBuilder(Storage &storage)
		: m_store(&storage), m_owned(false) {}

	/// Constructor that references a locally managed storage object.
	/// The pointer passed in will be stored, and freed when this class
	/// is destroyed.  It is safe to call this constructor with
	/// a 'new'ly created storage object.
	RecordBuilder(Storage *storage)
		: m_store(storage), m_owned(true) {}

	~RecordBuilder()
	{
		if( this->m_owned )
			delete m_store;
	}

	/// Functor member called by Controller::SaveDatabase() during
	/// processing.
	virtual bool operator()(Data &data, unsigned int databaseId)
	{
		Record rec;
		if( !(*m_storage)(rec) )
			return false;
		rec.Build(data, databaseId);
		return true;
	}
};

} // namespace Barry

#endif

