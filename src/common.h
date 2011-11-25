///
/// \file	common.h
///		General header for the Barry library
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#define VENDOR_RIM			0x0fca
#define PRODUCT_RIM_BLACKBERRY		0x0001
#define PRODUCT_RIM_PEARL_DUAL		0x0004
#define PRODUCT_RIM_PEARL_FLIP		0x8001		// 8200 series
#define PRODUCT_RIM_PEARL_8120		0x8004
#define PRODUCT_RIM_PEARL		0x0006
#define PRODUCT_RIM_STORM		0x8007
#define PRODUCT_RIM_PLAYBOOK_NETWORK	0x8011
#define PRODUCT_RIM_PLAYBOOK_STORAGE	0x8020

#define BLACKBERRY_INTERFACE		0
#define BLACKBERRY_CONFIGURATION	1
#define BLACKBERRY_MASS_STORAGE_CLASS	8
#define BLACKBERRY_DB_CLASS		0xff

#define IPRODUCT_RIM_HANDHELD		2
#define IPRODUCT_RIM_MASS_STORAGE	4
#define IPRODUCT_RIM_COMPOSITE		5

// minimum number of password tries remaining at which Barry gives up
// for safety
#define BARRY_MIN_PASSWORD_TRIES	3
#define BARRY_MIN_PASSWORD_TRIES_ASC	"3"

#define BLACKBERRY_CHARSET "WINDOWS-1252"

namespace Barry {

/// See also the LogLock class.
BXEXPORT void Init(bool data_dump_mode = false, std::ostream *logStream = &std::cout);
BXEXPORT void Verbose(bool data_dump_mode = true);
BXEXPORT bool IsVerbose();

} // namespace Barry

#endif

