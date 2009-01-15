///
/// \file	cod.h
///		COD structure
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    See also:
        http://drbolsen.wordpress.com/2006/07/26/blackberry-cod-file-format/
        http://drbolsen.wordpress.com/2006/08/11/10/

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

#include <stdint.h>
#include <sys/types.h>


typedef struct {
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


typedef struct {
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


typedef struct {
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

