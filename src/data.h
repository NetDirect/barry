///
/// \file	data.h
///		Class to deal with pre-saved data files
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

#ifndef __SB_DATA_H__
#define __SB_DATA_H__

#include "dll.h"
#include <iosfwd>
#include <vector>

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

} // namespace Barry

#endif

