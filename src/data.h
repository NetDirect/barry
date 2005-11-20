///
/// \file	data.h
///		Class to deal with pre-saved data files
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

#ifndef __SB_DATA_H__
#define __SB_DATA_H__

#include <iosfwd>
#include <vector>

class Data
{
	unsigned char *m_data;
	int m_bufsize;			//< size of m_data buffer allocated
	int m_datasize;			//< number of bytes of actual data
	int m_endpoint;

	// copy on write feature
	const unsigned char *m_externalData;
	bool m_external;

	// output format flags
	static bool bPrintAscii;

protected:
	void MakeSpace(int desiredsize);
	void CopyOnWrite(int desiredsize = -1);

public:
	Data();
	Data(int endpoint, int startsize = 0x4000);
	Data(const void *ValidData, int size);
	Data(const Data &other);
	~Data();

	void InputHexLine(std::istream &is);
	void DumpHexLine(std::ostream &os, int index, int size) const;
	void DumpHex(std::ostream &os) const;

	int GetEndpoint() const { return m_endpoint; }

	const unsigned char * GetData() const { return m_external ? m_externalData : m_data; }
	int GetSize() const { return m_datasize; }

	unsigned char * GetBuffer(int requiredsize = -1);
	int GetBufSize() const { return m_bufsize; }
	void ReleaseBuffer(int datasize = -1);

	void Zap();

	Data& operator=(const Data &other);


	// static functions
	static void PrintAscii(bool setting) { bPrintAscii = setting; }
	static bool PrintAscii() { return bPrintAscii; }
};

std::istream& operator>> (std::istream &is, Data &data);
std::ostream& operator<< (std::ostream &os, const Data &data);


class Diff
{
	const Data &m_old, &m_new;

	void Compare(std::ostream &os, int index, int size) const;

public:
	Diff(const Data &old, const Data &new_);

	void Dump(std::ostream &os) const;
};

std::ostream& operator<< (std::ostream &os, const Diff &diff);


// utility functions
bool LoadDataArray(const std::string &filename, std::vector<Data> &array);

#endif

