///
/// \file	cod-internal.h
///		COD structure
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2009, Josh Kropf
    See also:
        http://drbolsen.wordpress.com/2006/07/26/blackberry-cod-file-format/
        http://drbolsen.wordpress.com/2006/08/11/10/
        http://www.pkware.com/documents/casestudies/APPNOTE.TXT

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


#ifndef __BARRY_COD_INTERNAL_H__
#define __BARRY_COD_INTERNAL_H__

#include "dll.h"
#include <stdint.h>
#include <sys/types.h>

#define CODFILE_TYPE_SIMPLE	{0xDE, 0xC0}
#define CODFILE_TYPE_PKZIP	{0x50, 0x4B}
#define PKZIP_LOCAL_FILE_SIG	{0x50, 0x4B, 0x03, 0x04}
#define PKZIP_DIRECTORY_SIG	{0x50, 0x4B, 0x01, 0x02}
#define PKZIP_END_DIRECTORY_SIG	{0x50, 0x4B, 0x05, 0x06}


typedef struct BXLOCAL {
	uint16_t	hour:5;
	uint16_t	minute:6;
	uint16_t	second:5;
} __attribute__ ((packed)) msdos_time_t;


typedef struct BXLOCAL {
	uint16_t	year:7;		// number of years since 1980
	uint16_t	month:4;
	uint16_t	day:5;
} __attribute__ ((packed)) msdos_date_t;


typedef struct BXLOCAL {
	//uint8_t	signature[4];		// PKZIP local file header 0x504B0304
	uint16_t	version_needed;		// version needed to extract, 0x0A00
	uint16_t	general_flag;		// general purpose bit flag, 0x0000
	uint16_t	compression_method;	// compression method, 0x0000 = stored, no compression
	msdos_time_t	last_mod_time;
	msdos_date_t	last_mod_date;
	uint32_t	crc_32;
	uint32_t	compressed_size;	// compression method is 'stored'
	uint32_t	uncompressed_size;	// both sizes are equal
	uint16_t	file_name_length;
	uint16_t	extra_field_length;
	//char		file_name[variable];
	//char		extra_field[variable];
} __attribute__ ((packed)) pkzip_local_header_t;


typedef struct BXLOCAL {
	//uint8_t	signature[4];		// PKZIP central directory 0x504B0304
	uint16_t	version_madeby;		// version used to compress, 0x0A00
	uint16_t	version_needed;		// version needed to extract, 0x0A00
	uint16_t	general_flag;		// general purpose bit flag, 0x0000
	uint16_t	compression_method;	// compression method, 0x0000 = stored, no compression
	msdos_time_t	last_mod_time;
	msdos_date_t	last_mod_date;
	uint32_t	crc_32;
	uint32_t	compressed_size;	// size of corresponding local file entry
	uint32_t	uncompressed_size;	// both sizes are equal
	uint16_t	file_name_length;
	uint16_t	extra_field_length;
	uint16_t	file_comment_length;
	uint16_t	disk_number;		// number of the disk on which this file begins, always zero
	uint16_t	internal_file_attr;	// always zero
	uint32_t	external_file_attr;	// always zero
	uint32_t	relative_offset;	// offset from beginning of this disk (this zip file)
						// to start of corresponding local file entry
	//char		file_name[variable];
	//char		extra_field[variable];
	//char		file_comment[variable];
} __attribute__ ((packed)) pkzip_directory_t;


typedef struct BXLOCAL {
	//uint8_t	signature[4];		// PKZIP end central directory 0x504B0506
	uint16_t	this_disk_number;	// number of this disk, always zero
	uint16_t	disk_with_first;	// number of the disk with the start of
						// central directory, always zero
	uint16_t	this_disk_entry_count;	// total number of entries in the central directory on this disk
	uint16_t	total_entry_count;	// total number of entries in the central directory
						// always equals this_disk_entry_count
	uint32_t	directory_length;	// total size of the central directory
	uint32_t	directory_offset;	// offset from beginning of this disk (this zip file)
						// to the first central directory entry
	uint16_t	file_comment_length;
	//char		file_comment[variable];
} __attribute__ ((packed)) pkzip_end_directory_t;


typedef struct BXLOCAL {
	uint16_t 	type;			// Type			// 50 4B
	uint8_t		unknown1[8];		// 			// 03 04 0A 00 00 00 00 00
	uint8_t 	unknown2[4];		// 			// AB 5C 6A 39
	uint8_t 	unknown3[4];		// 			// BE 5C 58 D1
	uint32_t 	size1;			// COD size 0x0DCC	// CC 0D 01 00
	uint32_t 	size2;			// COD size 0x0DCC	// CC OD 01 00
	uint8_t 	strsize;		// Size of string	// 19
	uint8_t 	reserved2;		// Reserved 0x00	// 00
	uint8_t 	strfree;		// Empty uint8_t 	// 04
	uint8_t 	reserved3;		// Reserved 0x00	// 00
}  __attribute__ ((packed)) codfile_header_t;


typedef struct BXLOCAL {
	uint32_t	flashid;
	uint32_t	section_number;		// always 0
	uint32_t	vtable_pointer;		// always 0
	time_t		timestamp;
	uint32_t	user_version;
	uint32_t	fieldref_pointer;
	uint16_t	maxtype_list_size;
	uint16_t	reserved;		// always 0xFF
	int32_t		data_section;		// always 0xFFFF
	int32_t		module_info;		// always 0xFFFF
	uint16_t	version;
	uint16_t	code_size;
	uint16_t	data_size;
	uint16_t	flags;
}  __attribute__ ((packed)) code_header_t;


typedef struct BXLOCAL {
	uint8_t 	flags;
	uint8_t 	version;
	uint16_t 	num_icalls;
	uint8_t 	num_modules;
	uint8_t 	num_classes ;
	uint16_t 	exported_string_offset;
	uint16_t 	data_uint8_ts_offset;
	uint16_t 	empty_field;
	uint16_t 	class_definitions;
	uint16_t 	array_of_unknow1_fields[14];
	uint16_t 	aliases;
	uint16_t 	array_of_unknow2_fields[22];
}  __attribute__ ((packed)) data_header_t;

#endif

