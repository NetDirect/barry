///
/// \file	endian.h
///		Endian conversion macros
///

/*
    Copyright (C) 2006-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_ENDIAN_H__
#define __BARRY_ENDIAN_H__

// The Blackberry is little endian in its USB data.  Fortunately,
// this makes conversion easy on the x86...
#include "../config.h"

#ifndef WORDS_BIGENDIAN
#define btohs(x) x			// for uint16_t
#define btohl(x) x			// for uint32_t
#define btohll(x) x			// for uint64_t
#define htobs(x) x			// for uint16_t
#define htobl(x) x			// for uint32_t
#define htobll(x) x			// for uint64_t
#else
#include <byteswap.h>
#define btohs(x) bswap_16(x)		// for uint16_t
#define btohl(x) bswap_32(x)		// for uint32_t
#define btohll(x) bswap_64(x)		// for uint64_t
#define htobs(x) bswap_16(x)		// for uint16_t
#define htobl(x) bswap_32(x)		// for uint32_t
#define htobll(x) bswap_64(x)		// for uint64_t
#endif

#endif

