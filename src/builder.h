///
/// \file	builder.h
///		Virtual protocol packet builder wrapper
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

#ifndef __BARRY_BUILDER_H__
#define __BARRY_BUILDER_H__

#include "dll.h"
#include "data.h"
#include <stdint.h>
#include <string>

namespace Barry {

// forward declarations
class IConverter;

//
// Builder class
//
/// Base class for the builder functor hierarchy.
///
/// This defines the API used by the Controller and Packet classes
/// for building a raw device record to write to the device.
///
class BXEXPORT Builder
{
public:
	Builder() {}
	virtual ~Builder() {}

	/// Called first in the sequence, to allow the application to
	/// load the needed data from memory, disk, etc.  If successful,
	/// return true.  If at the end of the series, return false.
	/// Note that this function, if it returns true, may be called
	/// more than once, and it is the builder's responsibility
	/// to not retrieve the next item until BuildDone() is called.
	/// If Retrieve() returns false to mark the end-of-series,
	/// it does not have to return false next time, since a new
	/// series may begin.  It is the caller's responsibility to
	/// handle this.
	virtual bool Retrieve() = 0;

	/// Sometimes a builder can have multiple databases stored
	/// in it, so when Retrieve() returns false, check if there
	/// is more data with this function.  This function is
	/// not used by database-oriented functions, but by pipe-
	/// oriented functions.
	virtual bool EndOfFile() const = 0;

	/// Called to build the record field data.  Store the raw data
	/// in data, using offset to know where to write.  Be sure to
	/// update offset, and be sure to adjust the size of the data
	/// packet (possibly with Data::ReleaseBuffer()).
	virtual void BuildRecord(DBData &data, size_t &offset,
		const IConverter *ic) = 0;

	/// Called last to signify to the application that the library
	/// is finished with this record.  The next Retrieve() call
	/// can load the next item after this call.
	virtual void BuildDone() = 0;
};


//
// RecordBuilder template class
//
/// Template class for easy creation of specific protocol packet builder
/// objects.  This template takes the following template arguments:
///
///	- RecordT: One of the record classes in record.h
///	- StorageT: A custom storage functor class.  An object of this type
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
template <class RecordT, class StorageT>
class RecordBuilder : public Builder
{
	StorageT *m_storage;
	bool m_owned;
	bool m_record_loaded;
	bool m_end_of_file;
	RecordT m_rec;

public:
	/// Constructor that references an externally managed storage object.
	RecordBuilder(StorageT &storage)
		: m_storage(&storage)
		, m_owned(false)
		, m_record_loaded(false)
		, m_end_of_file(false)
	{
	}

	/// Constructor that references a locally managed storage object.
	/// The pointer passed in will be stored, and freed when this class
	/// is destroyed.  It is safe to call this constructor with
	/// a 'new'ly created storage object.
	RecordBuilder(StorageT *storage)
		: m_storage(storage)
		, m_owned(true)
		, m_record_loaded(false)
		, m_end_of_file(false)
	{
	}

	~RecordBuilder()
	{
		if( this->m_owned )
			delete m_storage;
	}

	virtual bool Retrieve()
	{
		if( m_end_of_file )
			return false;
		if( m_record_loaded )
			return true;

		m_record_loaded = (*m_storage)(m_rec, *this);
		if( !m_record_loaded )
			m_end_of_file = true;
		return m_record_loaded;
	}

	virtual bool EndOfFile() const
	{
		return m_end_of_file;
	}

	virtual void BuildRecord(DBData &data, size_t &offset,
				const IConverter *ic)
	{
		data.SetVersion(DBData::REC_VERSION_1);
		data.SetDBName(RecordT::GetDBName());
		data.SetIds(m_rec.GetRecType(), m_rec.GetUniqueId());
		data.SetOffset(offset);
		m_rec.BuildHeader(data.UseData(), offset);
		m_rec.BuildFields(data.UseData(), offset, ic);
	}

	virtual void BuildDone()
	{
		m_record_loaded = false;
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
	bool operator()(RecordT &rec, Builder &) const
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

