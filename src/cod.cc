///
/// \file	cod.cc
///		COD file API
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2008-2009, Nicolas VIVIEN

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

#include "cod.h"
#include "cod-internal.h"
#include "error.h"
#include "endian.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

using namespace std;

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// CodFile class

CodFile::CodFile(const char *filename)
	: m_fp(NULL)
	, m_filesize(0)
	, m_header(-1, sizeof(codfile_header_t))
{
	// Get file size
	struct stat sb;
	if (stat(filename, &sb) == -1) {
		throw Error(string("Can't stat: ") + filename);
	}

	m_filesize = sb.st_size;
	if( (unsigned long)m_filesize > (size_t)-1 ) {
		throw Error("Filesize larger than max fread()... contact Barry developers.");
	}

	// Open file
	m_fp = fopen(filename, "rb");
	if (m_fp == NULL) {
		throw Error(string("Can't open: ") + filename);
	}

}

CodFile::~CodFile()
{
	fclose(m_fp);
}

/// Returns true if block successfully read
bool CodFile::ReadNext()
{
	if( feof(m_fp) )
		return false;

	// read header
	size_t n = fread(m_header.GetBuffer(sizeof(codfile_header_t)),
		sizeof(codfile_header_t), 1, m_fp);
	if( n != 1 )
		return false;
	m_header.ReleaseBuffer(sizeof(codfile_header_t));

	const codfile_header_t *header = (const codfile_header_t*) m_header.GetData();

	// read block according to header type
	switch( GetType() )
	{
	case CODFILE_TYPE_SIMPLE:
		{
			rewind(m_fp);

			unsigned char *data = m_block.GetBuffer(m_filesize);
			n = fread(data, m_filesize, 1, m_fp);
			if( n != 1 )
				throw Error("Can't read COD data");

			m_block.ReleaseBuffer(m_filesize);
		}
		break;

	case CODFILE_TYPE_PACKED:
		{
			if (header->size1 != header->size2)
				return ReadNext();

			size_t skip = header->strsize + header->strfree;
			if( fseek(m_fp, skip, SEEK_CUR) != 0 ) {
				throw Error("Can't skip COD header");
			}

			size_t block_size = btohl(header->size1) * sizeof(char);
			unsigned char *data = m_block.GetBuffer(block_size);

			n = fread(data, block_size, 1, m_fp);
			if( n != 1 )
				throw Error("Can't read packed COD header");

			m_block.ReleaseBuffer(block_size);
		}
		break;

	default:
		throw Error("Unknown codfile header type.");
	}

	return true;
}

//
// Access current block
//
uint16_t CodFile::GetType() const
{
	const codfile_header_t *header = (const codfile_header_t*) m_header.GetData();
	return btohs(header->type);
}

} // namespace Barry

