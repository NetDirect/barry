/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <iostream>		// debugging only
#include <iomanip>
#include "common.h"
#include "log.h"

#ifndef __BARRY_DEBUG_H__	// only protect the non-macro portion, in order
#define __BARRY_DEBUG_H__	// to allow re-inclusion of debug.h with
				// different __DEBUG_MODE__ settings

namespace Barry {
extern bool __data_dump_mode__;
extern std::ostream *LogStream;
}

#endif // __BARRY_DEBUG_H__

// data dump output - controlled by command line -v switch
#define ddout(x)	if(Barry::__data_dump_mode__) { Barry::LogLock lock; (*Barry::LogStream) << x << std::endl; }

#ifdef __DEBUG_MODE__
	// debugging on
	#undef dout
	#undef eout

	// low level debug output
	#define dout(x)  	if(Barry::__data_dump_mode__) { Barry::LogLock lock; (*Barry::LogStream) << x << std::endl; }
//	#define dout(x)

	// exception output
	#define eout(x)  	{ Barry::LogLock lock; (*Barry::LogStream) << x << std::endl; }

	// easy exception output
	#define eeout(c, r)	{ Barry::LogLock lock; (*Barry::LogStream) << "Sent packet:\n" << c << "\n" << "Response packet:\n" << r << "\n"; }

	// handle assert()
	#undef NDEBUG
#else

	// debugging off
	#undef dout
	#undef eout

	#define dout(x)
	#define eout(x)  	{ Barry::LogLock lock; (*Barry::LogStream) << x << std::endl; }
	#define eeout(c, r)	{ Barry::LogLock lock; (*Barry::LogStream) << "Sent packet:\n" << c << "\n" << "Response packet:\n" << r << "\n"; }

	// handle assert() as well
	#define NDEBUG
#endif

#include <assert.h>

