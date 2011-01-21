///
/// \file	data.h
///		Class to deal with pre-saved data files
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __SB_DATA_H__
#define __SB_DATA_H__

#include "dll.h"
#include <iosfwd>
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

class BXEXPORT Data
{
	unsigned char *m_data;
	size_t m_bufsize;		//< size of m_data buffer allocated
	size_t m_datasize;		//< number of bytes of actual data
	int m_endpoint;

	// copy on write feature
	const unsigned char *m_externalData;
	bool m_external;

	// output format flags
	static bool bPrintAscii;

protected:
	void MakeSpace(size_t desiredsize);
	void CopyOnWrite(size_t desiredsize);

public:
	Data();
	explicit Data(int endpoint, size_t startsize = 0x4000);
	Data(const void *ValidData, size_t size);
	Data(const Data &other);
	~Data();

	void InputHexLine(std::istream &is);
	void DumpHexLine(std::ostream &os, size_t index, size_t size) const;
	void DumpHex(std::ostream &os) const;

	int GetEndpoint() const { return m_endpoint; }

	const unsigned char * GetData() const { return m_external ? m_externalData : m_data; }
	size_t GetSize() const { return m_datasize; }

	unsigned char * GetBuffer(size_t requiredsize = 0);
	size_t GetBufSize() const { return m_bufsize; }
	void ReleaseBuffer(int datasize = -1);

	void AppendHexString(const char *str);

	/// set buffer to 0 size, but don't bother overwriting memory with 0
	void QuickZap() { m_datasize = 0; }
	void Zap();	// does a memset too

	Data& operator=(const Data &other);


	//
	// Utility functions
	//
	// Writing data... basically does a memcpy(dst,src,sizeof(src))
	// for each type.  Does no endian conversions.
	// dst is calculated as buffer + offset.
	// The buffer is expanded automatically if needed.
	// The offset is advanced by the size of the data.
	//
	void MemCpy(size_t &offset, const void *src, size_t size);
	void Append(const void *buf, size_t size);
	template <class ValueT>
	void SetValue(size_t &offset, ValueT value)
	{
		this->MemCpy(offset, &value, sizeof(value));
	}


	// static functions
	static void PrintAscii(bool setting) { bPrintAscii = setting; }
	static bool PrintAscii() { return bPrintAscii; }
};

BXEXPORT std::istream& operator>> (std::istream &is, Data &data);
BXEXPORT std::ostream& operator<< (std::ostream &os, const Data &data);


class BXEXPORT Diff
{
	const Data &m_old, &m_new;

	BXLOCAL void Compare(std::ostream &os, size_t index, size_t size) const;

public:
	Diff(const Data &old, const Data &new_);

	void Dump(std::ostream &os) const;
};

BXEXPORT std::ostream& operator<< (std::ostream &os, const Diff &diff);


// utility functions
BXEXPORT bool LoadDataArray(const std::string &filename, std::vector<Data> &array);
BXEXPORT bool ReadDataArray(std::istream &is, std::vector<Data> &array);


//
// DBData
//
/// Database record data class.  The purpose of this class is to contain
/// the raw data that flows between low level activity such as device
/// read/writes, backup read/writes, and record parsing.
///
/// This class contains the low level record data block, unparsed, as well
/// as the surrounding meta data, such as the database name it belongs
/// to, the Unique ID, the Rec Type, and format version/type based on what
/// commands were used to extract the data from the device. (When using
/// newer commands, the format of the records, potentially including the
/// individual field type codes, are different.)
///
/// Possible bi-directional data flow in all of Barry:
/// Note that this class, DBData, represents the data+meta stage.
///
///	data+meta <-> device
///	data+meta <-> backup file
///	data+meta <-> record object
///	record object <-> boost serialization
///	contact record object <-> ldif
///
/// Possible uni-directional data flow in all of Barry:
///
///	record object -> text dump
///
class BXEXPORT DBData
{
public:
	enum RecordFormatVersion
	{
		REC_VERSION_1,
		REC_VERSION_2
	};

private:
	// record meta data
	RecordFormatVersion m_version;
	std::string m_dbName;
	uint8_t m_recType;
	uint32_t m_uniqueId;
	size_t m_offset;

	// the raw data block, internal
	Data *m_localData;

	// the data block to use... could be external or internal,
	// and does not change for the life of the object
	Data &m_data;

public:
	/// Default constructor, constructs an empty local Data object
	DBData();

	/// Constructs a local Data object that points to external memory
	DBData(const void *ValidData, size_t size);
	DBData(RecordFormatVersion ver, const std::string &dbName,
		uint8_t recType, uint32_t uniqueId, size_t offset,
		const void *ValidData, size_t size);

	/// If copy == false, constructs an external Data object, no local.
	/// If copy == true, constructs an internal Data object copy
	/// For speed, set copy to false.
	/// If you want Copy On Write behaviour, similar to Data(buf,size),
	/// then use the above (buf, size) constructor, not this one,
	/// since this constructor uses Data's copy constructor.
	DBData(Data &externalData, bool copy);
	DBData(RecordFormatVersion ver, const std::string &dbName,
		uint8_t recType, uint32_t uniqueId, size_t offset,
		Data &externalData, bool copy);

	~DBData();

	// access meta data
	RecordFormatVersion GetVersion() const { return m_version; }
	const std::string& GetDBName() const { return m_dbName; }
	uint8_t GetRecType() const { return m_recType; }
	uint32_t GetUniqueId() const { return m_uniqueId; }
	size_t GetOffset() const { return m_offset; }

	const Data& GetData() const { return m_data; }
	Data& UseData();

	// operations
	void SetVersion(RecordFormatVersion ver)
	{
		m_version = ver;
	}

	void SetDBName(const std::string &dbName)
	{
		m_dbName = dbName;
	}

	void SetIds(uint8_t recType, uint32_t uniqueId)
	{
		m_recType = recType;
		m_uniqueId = uniqueId;
	}

	void SetOffset(size_t offset)
	{
		m_offset = offset;
	}

	void CopyMeta(const DBData &src)
	{
		m_version = src.m_version;
		m_dbName = src.m_dbName;
		m_recType = src.m_recType;
		m_uniqueId = src.m_uniqueId;
		m_offset = src.m_offset;
	}

	DBData& operator=(const DBData &other);
};

} // namespace Barry

#endif

