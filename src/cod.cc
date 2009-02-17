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
#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <string.h>

#ifdef HAVE_ZLIB
 #include <zlib.h>
#endif

using namespace std;

namespace Barry {


uint32_t SeekNextCod(std::istream &input)
{
	char local_file_sig[] = PKZIP_LOCAL_FILE_SIG;
	char directory_sig[] = PKZIP_DIRECTORY_SIG;

	char signature[4];

	if( input.eof() )
		return 0;

	if( input.read(signature, sizeof(signature)).eof() ) {
		throw Error("SeekNextCod: EOF while reading file signature");
	}

	if( memcmp(signature, local_file_sig, sizeof(signature)) == 0 ) {
		pkzip_local_header_t header;

		if( input.read((char *)&header, sizeof(pkzip_local_header_t)).eof() ) {
			throw Error("SeekNextCod: EOF while reading PKZIP header");
		}

		// skip cod file name and extra field, we don't need them
		size_t skip_len = header.file_name_length + header.extra_field_length;
		if( input.ignore(skip_len).eof() ) {
			throw Error("SeekNextCod: EOF while skipping unused fields");
		}

		return btohl(header.compressed_size);
	}
	else if( memcmp(signature, directory_sig, sizeof(signature)) == 0 ) {
		// done reading when central directory is reached
		return 0;
	}
	else {
		// stream does not contain packed cod files, unget the 4 bytes
		for( unsigned int i=0; i<sizeof(signature); ++i ) input.unget();

		// find and return size of cod file

		if( input.seekg(0, ios::end).fail() ) {
			throw Error("SeekNextCod: seek to end failed");
		}

		uint32_t size = input.tellg();

		if( input.seekg(0, ios::beg).fail() ) {
			throw Error("SeekNextCod: seek to start failed");
		}

		return size;
	}
}


CodFileBuilder::CodFileBuilder(const std::string &module_name, size_t module_count)
	: m_module_name(module_name)
	, m_module_count(module_count)
	, m_current_module(0)
{
}

CodFileBuilder::~CodFileBuilder()
{
}

void CodFileBuilder::WriteNextHeader(std::ostream &output, const uint8_t* module_buffer, uint32_t module_size)
{
	// ignored for single module .cod files (simple .cod file)
	if( m_module_count == 1 ) {
		return;
	}

	// 32bit CRC of module in file header and directory entry
	// using zero for CRC will result in warnings when inflating .cod file
	uint32_t crc = 0;

#ifdef HAVE_ZLIB
	crc = crc32(0, NULL, module_size);
	crc = crc32(crc, module_buffer, module_size);
#endif

	// .cod file name for siblings have hyphenated index number, name-1.cod
	std::ostringstream file_name(m_module_name, ios::app);
	if( m_current_module == 0 )
		file_name << ".cod";
	else
		file_name << "-" << m_current_module << ".cod";

	// current stream pointer is relative offset to start of file entry
	uint32_t entry_offset = output.tellp();

	// structures for the local file entry and central directory entry
	pkzip_local_header_t header;
	pkzip_directory_t entry;

	// zero both structs, most fields are zero
	memset(&header, 0, sizeof(pkzip_local_header_t));
	memset(&entry, 0, sizeof(pkzip_directory_t));

	char header_sig[] = PKZIP_LOCAL_FILE_SIG;
	output.write(header_sig, sizeof(header_sig));

	// version is always 0x00A0 = 'Windows NTFS'
	header.version_needed = htobs(10);

	// time and date fields seem to randomly have invalid or fixed values
	// just leave them as zero
	//header.last_mod_time
	//header.last_mod_date

	header.crc_32 = htobl(crc);
	header.compressed_size = htobl(module_size);
	header.uncompressed_size = htobl(module_size);
	header.file_name_length = htobs(file_name.str().length());

	// the very first cod sibling to be written has an extra field
	// length equal to 4, with all zeros in the field itself
	// all subsequent siblings have a zero length extra field
	//header.extra_field_length = htobs(4);

	output.write((char *)&header, sizeof(pkzip_local_header_t));
	output << file_name.str();

	char footer_sig[] = PKZIP_DIRECTORY_SIG;

	// version is always 0x00A0 = 'Windows NTFS'
	entry.version_madeby = htobs(10);
	entry.version_needed = htobs(10);

	entry.crc_32 = htobl(crc);
	entry.compressed_size = htobl(module_size);
	entry.uncompressed_size = htobl(module_size);
	entry.file_name_length = htobs(file_name.str().length());
	entry.relative_offset = htobl(entry_offset);

	m_directory.write(footer_sig, sizeof(footer_sig));
	m_directory.write((char*)&entry, sizeof(pkzip_directory_t));
	m_directory << file_name.str();

	m_current_module ++;
}

void CodFileBuilder::WriteFooter(std::ostream &output)
{
	// ignored for single module .cod files (simple .cod file)
	if( m_module_count == 1 ) {
		return;
	}

	char sig[] = PKZIP_END_DIRECTORY_SIG;

	pkzip_end_directory_t end;
	memset(&end, 0, sizeof(pkzip_end_directory_t));

	end.this_disk_entry_count = htobs(m_current_module);
	end.total_entry_count = htobs(m_current_module);
	end.directory_length = m_directory.str().length();

	// current stream pointer is relative offset to start of directory
	end.directory_offset = output.tellp();

	output.write(m_directory.str().c_str(), m_directory.str().length());
	output.write(sig, sizeof(sig));
	output.write((char *)&end, sizeof(pkzip_end_directory_t));
}


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

