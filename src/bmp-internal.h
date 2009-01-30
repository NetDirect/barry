///
/// \file	bmp-internal.h
///		BMP structures
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    See also:
		http://www.fortunecity.com/skyscraper/windows/364/bmpffrmt.html

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


#ifndef __BARRY_BMP_INTERNAL_H__
#define __BARRY_BMP_INTERNAL_H__

#include "dll.h"
#include <stdint.h>
#include <sys/types.h>

typedef struct BXLOCAL {
	char bfType[2];				// Contains always 'BM'
	uint32_t bfSize;			// Size of file
	uint16_t bfReserved1;			// 0x00
	uint16_t bfReserved2;			// 0x00
	uint32_t bfOffBits;			// Offset to find the raw data
} __attribute__ ((packed)) bmp_file_header_t;

typedef struct BXLOCAL {
	uint32_t biSize;			// Size of struct itself
	uint32_t biWidth;			// Width of image
	uint32_t biHeight;			// Height of image
	uint16_t biPlanes;			//
	uint16_t biBitCount;			//
	uint32_t biCompression;			// 
	uint32_t biSizeImage;			// Size of raw data
	uint32_t biXPelsPerMeter;		//
	uint32_t biYPelsPerMeter;		//
	uint32_t biClrUsed;			//
	uint32_t biClrImportant;		//
} __attribute__ ((packed)) bmp_info_header_t;

#endif

