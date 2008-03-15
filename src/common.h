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

#define VENDOR_RIM		0x0fca
#define PRODUCT_RIM_BLACKBERRY	0x0001
#define PRODUCT_RIM_PEARL_DUAL	0x0004
#define PRODUCT_RIM_PEARL	0x0006

#define BLACKBERRY_CONFIGURATION	1
#define BLACKBERRY_DB_CLASS		0xff

namespace Barry {

BXEXPORT void Init(bool data_dump_mode = false);

} // namespace Barry

#endif

