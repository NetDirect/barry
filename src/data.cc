///
/// \file	data.cc
///		Classes to help manage pre-determined data files.
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

#include "data.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <locale>
#include "ios_state.h"

//#define __DEBUG_MODE__
#include "debug.h"


using namespace std;


namespace Barry {

inline bool IsHexData(const std::string &s)
{
	const char *str = s.c_str();
	for( int i = 0; i < 4 && *str; str++, i++ )
		if( *str != ' ' )
			return false;

	for( int i = 0; i < 8 && *str; str++, i++ ) {
		const char *hexchars = "0123456789abcdef";
		if( strchr(hexchars, *str) == NULL )
			return false;
	}

	if( *str != ':' )
		return false;

	return true;
}



///////////////////////////////////////////////////////////////////////////////
// Data class

bool Data::bPrintAscii = true;

Data::Data()
	: m_memBlock(new unsigned char[DEFAULT_SIZE + DEFAULT_PREPEND_SIZE])
	, m_blockSize(DEFAULT_SIZE + DEFAULT_PREPEND_SIZE)
	, m_dataStart(m_memBlock + DEFAULT_PREPEND_SIZE)
	, m_dataSize(0)
	, m_externalData(0)
	, m_external(false)
	, m_endpoint(-1)
{
	memset(m_memBlock, 0, m_blockSize);
}

Data::Data(int endpoint, size_t startsize, size_t prependsize)
	: m_memBlock(new unsigned char[startsize + prependsize])
	, m_blockSize(startsize + prependsize)
	, m_dataStart(m_memBlock + prependsize)
	, m_dataSize(0)
	, m_externalData(0)
	, m_external(false)
	, m_endpoint(endpoint)
{
	memset(m_memBlock, 0, m_blockSize);
}

Data::Data(const void *ValidData, size_t size)
	: m_memBlock(0)
	, m_blockSize(0)
	, m_dataStart(0)
	, m_dataSize(size)
	, m_externalData((const unsigned char*)ValidData)
	, m_external(true)
	, m_endpoint(-1)
{
}

Data::Data(const Data &other)
	: m_memBlock(other.m_blockSize ? new unsigned char[other.m_blockSize] : 0)
	, m_blockSize(other.m_blockSize)
	, m_dataStart(m_memBlock + other.AvailablePrependSpace())
	, m_dataSize(other.m_dataSize)
	, m_externalData(other.m_externalData)
	, m_external(other.m_external)
	, m_endpoint(other.m_endpoint)
{
	// copy over the raw data
	if( !m_external )
		memcpy(m_memBlock, other.m_memBlock, other.m_blockSize);
}

Data::~Data()
{
	delete [] m_memBlock;
}

//
// MakeSpace
//
/// Reallocates buffers so that it is safe to write desiredsize data
/// to m_dataStart after it returns.  All existing data is preserved.
///
/// This function also performs any copy on write needed.
///
/// If desiredprepend is nonzero, then at least desiredprepend bytes
/// of prepend space will exist in the buffer after return.
/// If desiredprepend is zero, defaults will be used if needed.
///
void Data::MakeSpace(size_t desiredsize, size_t desiredprepend)
{
	// use a default prepend size if none currently available
	size_t prepend = std::max(AvailablePrependSpace(), desiredprepend);
	if( !prepend )
		prepend = 0x100;

	// GetBufSize() returns 0 if m_external is true
	if( GetBufSize() < (desiredsize + prepend) ||
	    (desiredprepend && AvailablePrependSpace() < desiredprepend) )
	{
		// get a proper chunk to avoid future resizes
		desiredsize += 1024 + prepend;

		// desired size must be at least the size of our current
		// data (in case of external data), as well as the size
		// of our desired prepend space
		if( desiredsize < (m_dataSize + prepend) )
			desiredsize = m_dataSize + prepend;

		// setup new zeroed buffer... reuse m_memBlock if it
		// exists (see operator=())
		unsigned char *newbuf = 0;
		if( m_memBlock && m_blockSize >= desiredsize ) {
			newbuf = m_memBlock;
		}
		else {
			newbuf = new unsigned char[desiredsize];
			memset(newbuf, 0, desiredsize);
		}

		// copy valid data over
		if( m_external ) {
			memcpy(newbuf + prepend, m_externalData, m_dataSize);

			// not external anymore
			m_external = false;
		}
		else {
			memcpy(newbuf + prepend, m_dataStart, m_dataSize);
		}

		// install new buffer if we've allocated a new one
		if( m_memBlock != newbuf ) {
			delete [] m_memBlock;
			m_memBlock = newbuf;
			m_blockSize = desiredsize;
		}

		// update m_dataStart
		m_dataStart = m_memBlock + prepend;
	}
}

size_t Data::AvailablePrependSpace() const
{
	if( m_external )
		return 0;
	else
		return m_dataStart - m_memBlock;
}

void Data::InputHexLine(istream &is)
{
	ios_format_state state(is);

	unsigned int values[16];
	size_t index = 0;

	size_t address;
	is >> setbase(16) >> address;
	if( !is )
		return;		// nothing to do

	is.ignore();		// eat the ':'

	while( is && index < 16 ) {
		is >> setbase(16) >> values[index];
		if( is )
			index++;
	}

	dout("InputHexLine: read " << index << " bytes");

	MakeSpace(address + index);	// make space for the new
	m_dataSize = std::max(address + index, m_dataSize);
	while( index-- )
		m_dataStart[address + index] = (unsigned char) values[index];
	return;
}

void Data::DumpHexLine(ostream &os, size_t index, size_t size) const
{
	ios_format_state state(os);

	os.setf(ios::right);

	// index
	os << "    ";
	os << setbase(16) << setfill('0') << setw(8)
	   << index << ": ";

	// hex byte data
	for( size_t i = 0; i < size; i++ ) {
		if( (index+i) < GetSize() ) {
			os << setbase(16) << setfill('0')
			   << setw(2) << setprecision(2)
			   << (unsigned int) GetData()[index + i] << ' ';
		}
		else {
			os << "   ";
		}
	}

	// printable data
	if( bPrintAscii ) {
		locale loc = os.getloc();
		os << ' ';
		for( size_t i = 0; i < size && (index+i) < GetSize(); i++ ) {
			ostream::traits_type::char_type c = GetData()[index + i];
			os << setbase(10) << (char) (std::isprint(c, loc) ? c : '.');
		}
	}

	os << "\n";
}

void Data::DumpHex(ostream &os) const
{
	for( size_t address = 0; address < GetSize(); address += 16 ) {
		DumpHexLine(os, address, 16);
	}
}

unsigned char * Data::GetBuffer(size_t requiredsize)
{
	if( requiredsize == 0 ) {
		// handle default, use data size
		requiredsize = m_dataSize;
	}

	MakeSpace(requiredsize);
	return m_dataStart;
}

/// Returns size of buffer returned by GetBuffer().  Note that this does not
/// include available prepend space.
size_t Data::GetBufSize() const
{
	if( m_external )
		return 0;
	else
		return m_blockSize - (m_dataStart - m_memBlock);
}

void Data::ReleaseBuffer(int datasize)
{
	if( datasize < 0 && datasize != -1)
		throw std::logic_error("Data::ReleaseBuffer() argument must be -1 or >= 0");
	if( m_external )
		throw std::logic_error("Data::ReleaseBuffer() must be called after GetBuffer()");
	if( !(datasize == -1 || (unsigned int)datasize <= GetBufSize()) )
		throw std::logic_error("Data::ReleaseBuffer() must be called with a size smaller than the original buffer requested");

	if( datasize >= 0 ) {
		m_dataSize = datasize;
	}
	else {
		// search for last non-zero value in buffer
		m_dataSize = GetBufSize() - 1;
		while( m_dataSize && m_dataStart[m_dataSize] == 0 )
			--m_dataSize;
	}
}

/// Append bytes of data based on str
void Data::AppendHexString(const char *str)
{
	MakeSpace(m_dataSize + 512);

	std::istringstream iss(str);
	unsigned int byte;
	while( iss >> hex >> byte ) {
		MakeSpace(m_dataSize + 1);
		m_dataStart[m_dataSize] = (unsigned char) byte;
		m_dataSize++;
	}
}

/// set buffer to 0 and remove all data
void Data::Zap()
{
	if( !m_external )
		memset(m_memBlock, 0, m_blockSize);
	m_dataSize = 0;
}

Data & Data::operator=(const Data &other)
{
	if( this == &other )
		return *this;

	if( other.m_external ) {
		// just copy over the pointers
		m_externalData = other.m_externalData;
		m_external = other.m_external;
		m_dataSize = other.m_dataSize;
		m_endpoint = other.m_endpoint;
	}
	else {
		// don't remove our current buffer, only grow it if needed
		MakeSpace(other.m_dataSize);
		memcpy(m_dataStart, other.m_dataStart, other.m_dataSize);

		// then copy over the data state
		m_dataSize = other.m_dataSize;
		m_endpoint = other.m_endpoint;
	}

	return *this;
}

void Data::MemCpy(size_t &offset, const void *src, size_t size)
{
	unsigned char *pd = GetBuffer(offset + size) + offset;
	memcpy(pd, src, size);
	offset += size;

	// if the new end of data is larger than m_dataSize, bump it
	if( offset > m_dataSize )
		m_dataSize = offset;
}

void Data::Append(const void *buf, size_t size)
{
	// MemCpy updates m_datasize via the offset reference
	MemCpy(m_dataSize, buf, size);
}

void Data::Prepend(const void *buf, size_t size)
{
	MakeSpace(0, size);
	m_dataStart -= size;
	m_dataSize += size;
	memcpy(m_dataStart, (const unsigned char*) buf, size);
}

/// Removes size bytes from the beginning of the buffer.
/// If GetSize() is less than size, then all bytes will be chopped
/// and GetSize() will end up 0.
void Data::Prechop(size_t size)
{
	// chopping all the bytes that we have?
	if( size >= GetSize() ) {
		QuickZap();
		return;
	}

	if( m_external ) {
		m_externalData += size;
		m_dataSize -= size;
	}
	else {
		m_dataStart += size;
		m_dataSize -= size;
	}
}

istream& operator>> (istream &is, Data &data)
{
	data.InputHexLine(is);
	return is;
}

ostream& operator<< (ostream &os, const Data &data)
{
	data.DumpHex(os);
	return os;
}


///////////////////////////////////////////////////////////////////////////////
// Diff class

Diff::Diff(const Data &old, const Data &new_)
	: m_old(old), m_new(new_)
{
}

void Diff::Compare(ostream &os, size_t index, size_t size) const
{
	ios_format_state state(os);

	size_t min = std::min(m_old.GetSize(), m_new.GetSize());

	// index
	os << ">   ";
	os << setbase(16) << setfill('0') << setw(8)
	   << index << ": ";

	// diff data
	for( size_t i = 0; i < size; i++ ) {
		size_t address = index + i;

		// if data is available, print the diff
		if( address < min ) {
			if( m_old.GetData()[address] != m_new.GetData()[address] ) {
				// differ, print hex
				os << setbase(16) << setfill('0')
				   << setw(2) << setprecision(2)
				   << (unsigned int) m_new.GetData()[address] << ' ';
			}
			else {
				// same, just print spaces
				os << "   ";
			}
		}
		else {
			// one of the buffers is shorter...
			if( address < m_new.GetSize() ) {
				// new still has data, print it
				os << setbase(16) << setfill('0')
				   << setw(2) << setprecision(2)
				   << (unsigned int) m_new.GetData()[address]
				   << ' ';
			}
			else if( address < m_old.GetSize() ) {
				// new is out of data and old still has some
				os << "XX ";
			}
			else {
				// no more data, just print spaces
				os << "   ";
			}
		}
	}

	// printable data, just dump new
	if( Data::PrintAscii() ) {
		os << ' ';
		for( size_t i = 0; i < size && (index+i) < m_new.GetSize(); i++ ) {
			int c = m_new.GetData()[index + i];
			os << setbase(10) << (char) (isprint(c) ? c : '.');
		}
	}

	os << "\n";
}

void Diff::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	if( m_old.GetSize() != m_new.GetSize() )
		os << "sizes differ: "
		   << m_old.GetSize() << " != " << m_new.GetSize() << endl;

	size_t max = std::max(m_old.GetSize(), m_new.GetSize());
	for( size_t i = 0; i < max; i += 16 ) {
		m_old.DumpHexLine(os, i, 16);
		Compare(os, i, 16);
	}
}

ostream& operator<< (ostream &os, const Diff &diff)
{
	diff.Dump(os);
	return os;
}


///////////////////////////////////////////////////////////////////////////////
// DBData class

/// Default constructor, constructs an empty local Data object
DBData::DBData()
	: m_version(REC_VERSION_1)  // a reasonable default for now
	, m_localData(new Data)
	, m_data(*m_localData)
{
}

/// Copy constructor - always creates an internal Data object, and
/// uses Data object's copy constructor to make it.
/// Copies all meta data as well.
DBData::DBData(const DBData &other)
	: m_version(other.m_version)
	, m_dbName(other.m_dbName)
	, m_recType(other.m_recType)
	, m_uniqueId(other.m_uniqueId)
	, m_offset(other.m_offset)
	, m_localData(new Data(other.m_data))
	, m_data(*m_localData)
{
}

/// Constructs a local Data object that points to external memory
DBData::DBData(const void *ValidData, size_t size)
	: m_version(REC_VERSION_1)  // a reasonable default for now
	, m_localData(new Data)
	, m_data(*m_localData)
{
}

DBData::DBData(RecordFormatVersion ver,
		const std::string &dbName,
		uint8_t recType,
		uint32_t uniqueId,
		size_t offset,
		const void *ValidData,
		size_t size)
	: m_version(ver)
	, m_dbName(dbName)
	, m_recType(recType)
	, m_uniqueId(uniqueId)
	, m_offset(offset)
	, m_localData(new Data(ValidData, size))
	, m_data(*m_localData)
{
}

/// If copy == false, constructs an external Data object, no local.
/// If copy == true, constructs an internal Data object copy
DBData::DBData(Data &externalData, bool copy)
	: m_version(REC_VERSION_1)  // a reasonable default for now
	, m_localData(copy ? new Data(externalData) : 0)
	, m_data(copy ? *m_localData : externalData)
{
}

DBData::DBData(RecordFormatVersion ver,
		const std::string &dbName,
		uint8_t recType,
		uint32_t uniqueId,
		size_t offset,
		Data &externalData,
		bool copy)
	: m_version(ver)
	, m_dbName(dbName)
	, m_recType(recType)
	, m_uniqueId(uniqueId)
	, m_offset(offset)
	, m_localData(copy ? new Data(externalData) : 0)
	, m_data(copy ? *m_localData : externalData)
{
}

DBData::~DBData()
{
	delete m_localData;
}

Data& DBData::UseData()
{
	// make sure m_data is not external anymore
	m_data.GetBuffer();
	return m_data;	// return it
}

// Note: this copy operator does not change what m_data references...
// whatever m_data references in the constructor is what will be changed
// in this copy.
// Note also that the copy *will* involve a memcpy, and maybe a memory
// allocation as well.
DBData& DBData::operator=(const DBData &other)
{
	if( this == &other )
		return *this;

	// copy the data block
	m_data = other.m_data;

	// copy the metadata
	CopyMeta(other);

	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Utility functions

static bool IsEndpointStart(const std::string &line, int &endpoint)
{
	if( strncmp(line.c_str(), "sep: ", 5) == 0 ||
	    strncmp(line.c_str(), "rep: ", 5) == 0 )
	{
		endpoint = atoi(line.c_str() + 5);
		return true;
	}
	return false;
}

bool LoadDataArray(const string &filename, std::vector<Data> &array)
{
	ifstream in(filename.c_str());
	return ReadDataArray(in, array);
}

bool ReadDataArray(std::istream &is, std::vector<Data> &array)
{
	if( !is )
		return false;

	bool bInEndpoint = false;
	unsigned int nCurrent = 0;
	size_t nLargestSize = 0x100;
	while( is ) {
		string line;
		getline(is, line);
		int endpoint;
		if( bInEndpoint ) {
			if( IsHexData(line) ) {
				istringstream sline(line);
				sline >> array[nCurrent];
				continue;
			}
			else {
				nLargestSize = std::max(nLargestSize,
					array[nCurrent].GetBufSize());
				bInEndpoint = false;
			}
		}

		// check if this line starts a new endpoint
		if( IsEndpointStart(line, endpoint) ) {
			bInEndpoint = true;
			Data chunk(endpoint, nLargestSize);
			array.push_back(chunk);
			nCurrent = array.size() - 1;
		}
	}
	return true;
}

} // namespace Barry

