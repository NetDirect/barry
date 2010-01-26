///
/// \file	strnlen.h
///		Header for strnlen() call, for systems that don't have GNU.
///

/*
    Copyright (C) 2007-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_STRNLEN_H__
#define __BARRY_STRNLEN_H__

#include "config.h"
#include <string.h>

// this is always defined by configure, if not, then the autoconf
// sources changed (likely in /usr/share/autoconf/autoconf/functions.m4)
// and configure.ac is no longer accurate
#ifndef HAVE_WORKING_STRNLEN
#error Configure.ac is not accurate. Read comments in strnlen.h.
#endif

// now, if defined and set to 0, then strnlen() either does not exist at all,
// or is broken (on AIX)
#if !HAVE_WORKING_STRNLEN

// so define our own version...
#ifdef __cplusplus
extern "C" {
#endif

size_t barry_strnlen(const char *s, size_t maxlen);

#ifdef __cplusplus
}
#endif

// and override the system's name so we call our own
#define strnlen(s,l) barry_strnlen(s,l)

#endif	// !HAVE_WORKING_STRNLEN

#endif	// __BARRY_STRNLEN_H__

