///
/// \file	builder.h
///		Virtual protocol packet builder wrapper
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

//
// This macro can be used to automatically generate code for all known
// record types.  Just #undef HANDLE_BUILDER, then #define it to whatever
// you need, then use ALL_KNOWN_BUILDER_TYPES.  See parser.cc for
// various examples.
//
// These are sorted so their GetDBName()'s will display in alphabetical order.
//
#define ALL_KNOWN_BUILDER_TYPES \
	HANDLE_BUILDER(Contact) \
	HANDLE_BUILDER(Calendar) \
	HANDLE_BUILDER(CalendarAll) \
	HANDLE_BUILDER(ContentStore) \
	HANDLE_BUILDER(Memo) \
	HANDLE_BUILDER(Task) \

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

	/// Called to build the record field data.  Store the raw data
	/// in data, using offset to know where to write.  Be sure to
	/// update offset, and be sure to adjust the size of the data
	/// packet (possibly with Data::ReleaseBuffer()).
	///
	/// Returns true if successful, and false if at the end of
	/// the series.  Note that if EndOfFile() is false after
	/// this function returns false, then there may be another
	/// series available, which the next call to BuildRecord()
	/// will determine.
	///
	virtual bool BuildRecord(DBData &data, size_t &offset,
		const IConverter *ic) = 0;

	/// Same as BuildRecord, but does not care about any offsets.
	/// The caller should call DBData::GetOffset() afterward
	/// to discover if there is an offset to the result.
	///
	/// This is usually the fastest of the two functions, since
	/// extra copying may be required if a specific offset is
	/// given.  When building records from Record classes, both
	/// functions are the same speed.  But when building records
	/// from the device, the device decides the offset, so FetchRecord()
	/// is faster, since BuildRecord requires a copy to adjust
	/// to the right offset.
	///
	/// The caller should use the function that results in the least
	/// amount of copying for the caller.  If the caller doesn't
	/// care about where the resulting record is in data, use
	/// FetchRecord().
	///
	virtual bool FetchRecord(DBData &data, const IConverter *ic) = 0;

	/// Sometimes a builder can have multiple databases stored
	/// in it, so when Build/Fetch returns false, check if there
	/// is more data with this function.  This function is
	/// not used by database-oriented functions, but by pipe-
	/// oriented functions.
	virtual bool EndOfFile() const = 0;
};


//
// DBDataBuilder
//
/// Wrapper class around a DBData object, to make it easy to pass a DBData
/// object into a function or API that requires a builder.  The main
/// advantage to this is that the Builder API allows for movement of
/// data, depending on the required offsets.
///
class BXEXPORT DBDataBuilder : public Builder
{
	const DBData &m_orig;

public:
	explicit DBDataBuilder(const DBData &orig);
	virtual ~DBDataBuilder();

	virtual bool BuildRecord(DBData &data, size_t &offset,
		const IConverter *ic);
	virtual bool FetchRecord(DBData &data, const IConverter *ic);
	virtual bool EndOfFile() const;
};

//
// SetDBData
//
/// Contains the proper way to convert a record object into a DBData object.
///
template <class RecordT>
void SetDBData(const RecordT &rec, DBData &data, size_t &offset,
		const IConverter *ic)
{
	data.SetVersion(DBData::REC_VERSION_1);
	data.SetOffset(offset);
	data.SetDBName(RecordT::GetDBName());
	data.SetIds(rec.GetRecType(), rec.GetUniqueId());
	rec.BuildHeader(data.UseData(), offset);
	rec.BuildFields(data.UseData(), offset, ic);
}

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

	virtual bool BuildRecord(DBData &data, size_t &offset,
				const IConverter *ic)
	{
		if( m_end_of_file )
			return false;

		if( !(*m_storage)(m_rec, *this) ) {
			m_end_of_file = true;
			return false;
		}

		SetDBData(m_rec, data, offset, ic);
		return true;
	}

	virtual bool FetchRecord(DBData &data, const IConverter *ic)
	{
		size_t offset = 0;
		return BuildRecord(data, offset, ic);
	}

	virtual bool EndOfFile() const
	{
		return m_end_of_file;
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

