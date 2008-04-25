///
/// \file	common.cc
///		General Barry interface routines
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

#include "common.h"
#include <usb.h>
#include <pthread.h>
#include "debug.h"

namespace Barry {

bool __data_dump_mode__;

std::ostream *LogStream = &std::cout;
pthread_mutex_t LogStreamMutex;


//
// Init
//
/// Barry library initializer.  Call this before anything else.
/// This takes care of initializing the lower level libusb.
///
/// \param[in]	data_dump_mode	If set to true, the protocol conversation
///				will be sent to stdout via the C++ std::cout
///				stream.
/// \param[in]	LogStream	Pointer to std::ostream object to use for
///				debug output and logging.  Defaults to
///				std::cout.
///
void Init(bool data_dump_mode, std::ostream *logStream)
{
	if( data_dump_mode )
		usb_set_debug(9);
	usb_init();

	__data_dump_mode__ = data_dump_mode;
	LogStream = logStream;
	pthread_mutex_init(&LogStreamMutex, NULL);
}

} // namespace Barry

