//
// \file	format_check.h
//		Simple macro to enable gcc printf-style format string checks
//

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_FORMAT_CHECK_H__
#define __BARRY_SYNC_FORMAT_CHECK_H__

#if __GNUC__
#define BARRY_GCC_FORMAT_CHECK(a,b) __attribute__ ((format(printf, a, b)))
#else
#define BARRY_GCC_FORMAT_CHECK(a,b)
#endif

#endif

