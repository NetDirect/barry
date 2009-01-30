///
/// \file	cod.h
///		COD file API
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_COD_H__
#define __BARRY_COD_H__

#include "dll.h"
#include "data.h"
#include <stdio.h>

#define CODFILE_TYPE_PACKED	0x4B50
#define CODFILE_TYPE_SIMPLE	0xC0DE

namespace Barry {

class BXEXPORT CodFile
{
	FILE *m_fp;
	off_t m_filesize;
	Data m_header;
	Data m_block;

public:
	explicit CodFile(const char *filename);
	~CodFile();

	/// Returns true if block successfully read
	bool ReadNext();

	//
	// Access current block
	//
	uint16_t GetType() const;
	const Data& GetHeader()		{ return m_header; }
	const Data& GetBlock()		{ return m_block; }
};

}

#endif

