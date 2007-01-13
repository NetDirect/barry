///
/// \file	builder.h
///		Virtual protocol packet builder wrapper
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

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
/// This defines the API used by the Controller and Packet classes
/// for building a raw device record to write to the device.
///
class Builder
{
public:
	Builder() {}
	virtual ~Builder() {}

	/// Called first in the sequence, to allow the application to
	/// load the needed data from memory, disk, etc.  If successful,
	/// return true.  If at the end of the series, return false.
	virtual bool Retrieve(unsigned int databaseId) = 0;

	/// Called to retrive the unique ID for this record.
	virtual uint32_t GetUniqueId() const = 0;

	/// Called before BuildFields() in order to build the header
	/// for this record.  Store the raw data in data, at the
	/// offset given in offset.  When finished, update offset to
	/// point to the next spot to put new data.
	virtual void BuildHeader(Data &data, size_t &offset) = 0;

	/// Called to build the record field data.  Store the raw data
	/// in data, using offset to know where to write.  Be sure to
	/// update offset, and be sure to adjust the size of the data
	/// packet (possibly with Data::ReleaseBuffer()).
	virtual void BuildFields(Data &data, size_t &offset) = 0;
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
///		argument.  The storage class is expected to fill the
///		record object in preparation for building the packet
///		out of that data.  These calls happen on the fly as the data
///		is sent to the device over USB, so it should not block forever.
///
/// Example SaveDatabase() call:
///
/// <pre>
/// FIXME
/// </pre>
///
template <class Record, class Storage>
class RecordBuilder : public Builder
{
	Storage *m_storage;
	bool m_owned;
	Record m_rec;

public:
	/// Constructor that references an externally managed storage object.
	RecordBuilder(Storage &storage)
		: m_storage(&storage), m_owned(false) {}

	/// Constructor that references a locally managed storage object.
	/// The pointer passed in will be stored, and freed when this class
	/// is destroyed.  It is safe to call this constructor with
	/// a 'new'ly created storage object.
	RecordBuilder(Storage *storage)
		: m_storage(storage), m_owned(true) {}

	~RecordBuilder()
	{
		if( this->m_owned )
			delete m_storage;
	}

	virtual bool Retrieve(unsigned int databaseId)
	{
		return (*m_storage)(m_rec, databaseId);
	}

	virtual uint32_t GetUniqueId() const
	{
		return m_rec.GetUniqueId();
	}

	/// Functor member called by Controller::SaveDatabase() during
	/// processing.
	virtual void BuildHeader(Data &data, size_t &offset)
	{
		m_rec.BuildHeader(data, offset);
	}

	virtual void BuildFields(Data &data, size_t &offset)
	{
		m_rec.BuildFields(data, offset);
	}
};


//
// RecordFetch template class
//
/// Generic record fetch class, to help with using records without
/// builder classes.
///
template <class RecordT>
class RecordFetch
{
	const RecordT &m_rec;
	mutable bool m_done;

public:
	RecordFetch(const RecordT &rec) : m_rec(rec), m_done(false) {}
	bool operator()(RecordT &rec, unsigned int dbId) const
	{
		if( m_done )
			return false;
		rec = m_rec;
		m_done = true;
		return true;
	}
};


} // namespace Barry

#endif

