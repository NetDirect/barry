///
/// \file	barrygetopt.h
///		Simple wrapper header to include the correct #include for
///		getopt
///

/*
    Copyright (C) 2011, RealVNC Ltd.

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

#ifndef __BARRY_GETOPT_H__
#define __BARRY_GETOPT_H__

#if defined(__QNX__) || defined(WINCE)
#include <unistd.h>
#else
#include <getopt.h>
#endif

#endif // __BARRY_GETOPT_H__
