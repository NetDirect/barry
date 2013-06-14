///
/// \file	clog.h
///		C oriented logging routines for Barry
///

/*
    Copyright (C) 2010-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_CLOG_H__
#define __BARRY_CLOG_H__

#include "dll.h"

#ifdef __cplusplus
extern "C" {
#endif

BXEXPORT void BarryLogf(int verbose, const char *msg, ...) BARRY_GCC_FORMAT_CHECK(2, 3);

#ifdef __cplusplus
}
#endif

#endif // __BARRY_CLOG_H__

