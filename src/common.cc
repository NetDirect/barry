///
/// \file	common.cc
///		General Barry interface routines
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <pthread.h>
#include "debug.h"
#include "config.h"

#ifdef USE_BARRY_SOCKETS
#include "usbwrap.h"
#endif

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
/// This function is safe to be called multiple times.  The
/// data_dump_mode and the log stream will be updated each time
/// it is called, but the USB library will not be re-initialized.
///
/// \param[in]	data_dump_mode	If set to true, the protocol conversation
///				will be sent to the logStream specified
///				in the second argument.
/// \param[in]	LogStream	Pointer to std::ostream object to use for
///				debug output and logging.  Defaults to
///				std::cout.
///
void Init(bool data_dump_mode, std::ostream *logStream)
{
	static bool initialized = false;

#ifdef USE_BARRY_SOCKETS
	Usb::LibraryInterface::SetDataDump(data_dump_mode);
#endif

	// perform one-time initalization
	if( !initialized ) {
#ifdef USE_BARRY_SOCKETS
		// Should call Usb::Uninit at some point,
		// but there isn't currently a deinit call.
		int err = 0;
		if( !Usb::LibraryInterface::Init(&err) ) {
			eout("USB library failed to initialise with libusb error: " << err);
			throw Error("Failed to initialise USB");
			return;
		}
#endif

		// only need to initialize this once
		pthread_mutex_init(&LogStreamMutex, NULL);

		// done
		initialized = true;
	}

	__data_dump_mode__ = data_dump_mode;
	LogStream = logStream;
}

//
// Verbose
//
/// This API call lets the application enable / disable verbose debug
/// output on the fly.
///
/// \param[in]	data_dump_mode	If set to true, the protocol conversation
///				will be sent to the logStream specified
///				in the Barry::Init() call.
///
void Verbose(bool data_dump_mode)
{
	__data_dump_mode__ = data_dump_mode;

#ifdef USE_BARRY_SOCKETS
	Usb::LibraryInterface::SetDataDump(data_dump_mode);
#endif
}

//
// IsVerbose
//
/// Returns true if data dump mode is enabled.
///
bool IsVerbose()
{
	return __data_dump_mode__;
}

} // namespace Barry

