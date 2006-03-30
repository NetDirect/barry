///
/// \file	data.cc
///		Classes to help manage pre-determined data files.
///

/*
    Copyright (C) 2005-2006, Net Direct Inc. (http://www.netdirect.ca/)

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

//#define __DEBUG_MODE__
#include "debug.h"


using namespace std;


inline bool IsHexData(const std::string &s)
{
	const char *str = s.c_str();
	for( int i = 0; i < 4 && *str; str++, i++ )
		if( *str != ' ' )
			return false;

	for( int i = 0; i < 8 && *str; str++, i++ )
		if( !isdigit(*str) && !(*str >= 'a' && *str <= 'f') )
			return false;

	if( *str != ':' )
		return false;

	return true;
}



///////////////////////////////////////////////////////////////////////////////
// Data class

bool Data::bPrintAscii = true;

Data::Data()
	: m_data(new unsigned char[0x4000]),
	m_bufsize(0x4000),
	m_datasize(0),
	m_endpoint(-1),
	m_externalData(0),
	m_external(false)
{
	memset(m_data, 0, m_bufsize);
}

Data::Data(int endpoint, size_t startsize)
	: m_data(new unsigned char[startsize]),
	m_bufsize(startsize),
	m_datasize(0),
	m_endpoint(endpoint),
	m_externalData(0),
	m_external(false)
{
	memset(m_data, 0, m_bufsize);
}

Data::Data(const void *ValidData, size_t size)
	: m_data(0),
	m_bufsize(0),
	m_datasize(size),
	m_endpoint(-1),
	m_externalData((const unsigned char*)ValidData),
	m_external(true)
{
}

Data::Data(const Data &other)
	: m_data(other.m_bufsize ? new unsigned char[other.m_bufsize] : 0),
	m_bufsize(other.m_bufsize),
	m_datasize(other.m_datasize),
	m_endpoint(other.m_endpoint),
	m_externalData(other.m_externalData),
	m_external(other.m_external)
{
	// copy over the raw data
	if( !m_external )
		memcpy(m_data, other.m_data, other.m_bufsize);
}

Data::~Data()
{
	delete [] m_data;
}

void Data::MakeSpace(size_t desiredsize)
{
	if( m_bufsize < desiredsize ) {
		desiredsize += 1024;	// get a proper chunk
		unsigned char *newbuf = new unsigned char[desiredsize];
		memcpy(newbuf, m_data, m_bufsize);
		memset(newbuf + m_bufsize, 0, desiredsize - m_bufsize);
		delete [] m_data;
		m_data = newbuf;
		m_bufsize = desiredsize;
	}
}

// perform the copy on write operation if needed
void Data::CopyOnWrite(size_t desiredsize)
{
	if( m_external ) {
		// make room
		MakeSpace(std::max(desiredsize, m_datasize));

		// copy it over
		memcpy(m_data, m_externalData, m_datasize);

		// not external anymore
		m_external = false;
	}
}

void Data::InputHexLine(istream &is)
{
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

	CopyOnWrite(address + index);
	MakeSpace(address + index);	// make space for the new
	m_datasize = std::max(address + index, m_datasize);
	while( index-- )
		m_data[address + index] = (unsigned char) values[index];
	return;
}

void Data::DumpHexLine(ostream &os, size_t index, size_t size) const
{
	ios::fmtflags oldflags = os.setf(ios::right);

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
		os << ' ';
		for( size_t i = 0; i < size && (index+i) < GetSize(); i++ ) {
			int c = GetData()[index + i];
			os << setbase(10) << (char) (isprint(c) ? c : '.');
		}
	}

	os << "\n";
	os.flags(oldflags);
}

void Data::DumpHex(ostream &os) const
{
	for( size_t address = 0; address < GetSize(); address += 16 ) {
		DumpHexLine(os, address, 16);
	}
}

unsigned char * Data::GetBuffer(size_t requiredsize)
{
	CopyOnWrite(requiredsize);
	if( requiredsize > 0 )
		MakeSpace(requiredsize);
	return m_data;
}

void Data::ReleaseBuffer(int datasize)
{
	assert( datasize <= m_bufsize );
	assert( datasize >= 0 || datasize == -1 );
	assert( !m_external );

	if( datasize >= 0 ) {
		m_datasize = datasize;
	}
	else {
		// search for last non-zero value in buffer
		m_datasize = m_bufsize - 1;
		while( m_datasize && m_data[m_datasize] == 0 )
			--m_datasize;
	}
}

/// set buffer to 0 and remove all data
void Data::Zap()
{
	if( !m_external )
		memset(m_data, 0, m_bufsize);
	m_datasize = 0;
}

Data & Data::operator=(const Data &other)
{
	if( this == &other )
		return *this;

	// don't remove our current buffer, only grow it if needed
	MakeSpace(other.m_bufsize);
	memcpy(m_data, other.m_data, other.m_bufsize);

	// then copy over the data state
	m_datasize = other.m_datasize;
	m_endpoint = other.m_endpoint;
	m_externalData = other.m_externalData;
	m_external = other.m_external;
	return *this;
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
	if( !in )
		return false;

	bool bInEndpoint = false;
	unsigned int nCurrent = 0;
	size_t nLargestSize = 0x100;
	while( in ) {
		string line;
		getline(in, line);
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


#ifdef __TEST_MODE__

#include <iostream>
#include <iomanip>
#include "data.h"

using namespace std;

int main()
{
	typedef std::vector<Data> DataVec;
	DataVec array;
	if( !LoadDataArray("data/parsed.log", array) ) {
		cout << "Can't load file" << endl;
		return 1;
	}

	DataVec::iterator i = array.begin();
	Data::PrintAscii(false);
	for( ; i != array.end(); i++ ) {
		cout << "Endpoint: " << i->GetEndpoint() << endl;
		cout << *i;
		cout << "\n\n";
	}


	Data one, two;
	one.GetBuffer()[0] = 0x01;
	one.ReleaseBuffer(1);
	two.GetBuffer()[0] = 0x02;
	two.ReleaseBuffer(2);

	cout << Diff(one, two) << endl;
	cout << Diff(two, one) << endl;

	two.GetBuffer();
	two.ReleaseBuffer(32);
	cout << Diff(one, two) << endl;
}

#endif

