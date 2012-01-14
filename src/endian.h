///
/// \file	endian.h
///		Endian conversion macros
///

/*
    Copyright (C) 2006-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "config.h"		// endian.h is not installed, so this is safe
#include <stdint.h>

//#include <byteswap.h>

// The following is a byteswap.h replacement, for systems like Mac OS X.
// It was taken from a patch to the GPL software cowpatty, patch
// by user gm2net.
// http://www.netstumbler.org/showpost.php?s=79764fd1526e4653d5cb4432225da6ee&p=190494&postcount=29

//#warning "byteswap.h is an unportable GNU extension!  Don't use!"
static inline uint16_t bbswap_16(uint16_t x) {
  return (x>>8) | (x<<8);
}

static inline uint32_t bbswap_32(uint32_t x) {
  return (bbswap_16(x&0xffff)<<16) | (bbswap_16(x>>16));
}

static inline uint64_t bbswap_64(uint64_t x) {
  return (((uint64_t)bbswap_32(x&0xffffffffull))<<32) | (bbswap_32(x>>32));
}

#ifndef WORDS_BIGENDIAN

// For when Blackberry needs little endian (most of the time)
#define btohs(x) x			// for uint16_t
#define btohl(x) x			// for uint32_t
#define btohll(x) x			// for uint64_t
#define htobs(x) x			// for uint16_t
#define htobl(x) x			// for uint32_t
#define htobll(x) x			// for uint64_t

// For when Blackberry needs big endian (often in JavaLoader protocol)
#define be_btohs(x) bbswap_16(x)	// for uint16_t
#define be_btohl(x) bbswap_32(x)	// for uint32_t
#define be_btohll(x) bbswap_64(x)	// for uint64_t
#define be_htobs(x) bbswap_16(x)	// for uint16_t
#define be_htobl(x) bbswap_32(x)	// for uint32_t
#define be_htobll(x) bbswap_64(x)	// for uint64_t

#else

// For when Blackberry needs little endian (most of the time)
#define btohs(x) bbswap_16(x)		// for uint16_t
#define btohl(x) bbswap_32(x)		// for uint32_t
#define btohll(x) bbswap_64(x)		// for uint64_t
#define htobs(x) bbswap_16(x)		// for uint16_t
#define htobl(x) bbswap_32(x)		// for uint32_t
#define htobll(x) bbswap_64(x)		// for uint64_t

// For when Blackberry needs big endian (often in JavaLoader protocol)
#define be_btohs(x) x			// for uint16_t
#define be_btohl(x) x			// for uint32_t
#define be_btohll(x) x			// for uint64_t
#define be_htobs(x) x			// for uint16_t
#define be_htobl(x) x			// for uint32_t
#define be_htobll(x) x			// for uint64_t

#endif

#endif

