/** 
 * @file parser.h
 * @author Nicolas VIVIEN
 * @date 2009-08-01
 *
 * @note CopyRight Nicolas VIVIEN
 *
 * @brief COD debug file parser
 *
 * @par Modifications
 *   - 2009/08/01 : N. VIVIEN
 *     - First release
 *
 * @par Licences
 *   Copyright (C) 2009-2010, Nicolas VIVIEN
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 *   See the GNU General Public License in the COPYING file at the
 *   root directory of this project for more details.
 */


#ifndef __BARRYJDG_PARSER_H__
#define __BARRYJDG_PARSER_H__

#include <string>
#include <stdint.h>


namespace Barry {

namespace JDG {

// The following is a byteswap.h replacement, for systems like Mac OS X.
// It was taken from a patch to the GPL software cowpatty, patch
// by user gm2net.
// http://www.netstumbler.org/showpost.php?s=79764fd1526e4653d5cb4432225da6ee&p=190494&postcount=29

//#warning "byteswap.h is an unportable GNU extension!  Don't use!"

static inline unsigned short bswap_16(unsigned short x) {
  return (x>>8) | (x<<8);
}

static inline unsigned int bswap_32(unsigned int x) {
  return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16));
}

static inline uint64_t bswap_64(uint64_t x) {
  return (((uint64_t)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32));
}

#ifndef WORDS_BIGENDIAN

// For when Blackberry needs big endian (often in JavaLoader protocol)
#define be_btohs(x) bswap_16(x)		// for uint16_t
#define be_btohl(x) bswap_32(x)		// for uint32_t
#define be_btohll(x) bswap_64(x)	// for uint64_t
#define be_htobs(x) bswap_16(x)		// for uint16_t
#define be_htobl(x) bswap_32(x)		// for uint32_t
#define be_htobll(x) bswap_64(x)	// for uint64_t

#else

// For when Blackberry needs big endian (often in JavaLoader protocol)
#define be_btohs(x) x			// for uint16_t
#define be_btohl(x) x			// for uint32_t
#define be_btohll(x) x			// for uint64_t
#define be_htobs(x) x			// for uint16_t
#define be_htobl(x) x			// for uint32_t
#define be_htobll(x) x			// for uint64_t

#endif


std::string ParseString(std::istream &input, const int length);
uint32_t ParseInteger(std::istream &input);

} // namespace JDG

} // namespace Barry

#endif

