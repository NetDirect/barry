///
/// \file	common.h
///		General header for the Barry library
///

/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_COMMON_H__
#define __BARRY_COMMON_H__

#include "dll.h"
#include <iostream>

#define VENDOR_RIM		0x0fca
#define PRODUCT_RIM_BLACKBERRY	0x0001
#define PRODUCT_RIM_PEARL_DUAL	0x0004
#define PRODUCT_RIM_PEARL_8120	0x8004
#define PRODUCT_RIM_PEARL	0x0006

#define BLACKBERRY_CONFIGURATION	1
#define BLACKBERRY_DB_CLASS		0xff

// minimum number of password tries remaining at which Barry gives up
// for safety
#define BARRY_MIN_PASSWORD_TRIES	3
#define BARRY_MIN_PASSWORD_TRIES_ASC	"3"

namespace Barry {

/// See also the LogLock class.
BXEXPORT void Init(bool data_dump_mode = false, std::ostream *logStream = &std::cout);

} // namespace Barry

#endif

