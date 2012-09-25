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

#include "i18n.h"
#include "common.h"
#include "platform.h"
#include <pthread.h>
#include "debug.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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
		// initialize i18n gettext directory
		// the rest is done in i18n.h
		bindtextdomain(PACKAGE, LOCALEDIR);

#ifdef USE_BARRY_SOCKETS
		// Should call Usb::Uninit at some point,
		// but there isn't currently a deinit call.
		int err = 0;
		if( !Usb::LibraryInterface::Init(&err) ) {
			eout(_("USB library failed to initialise with libusb error: ") << err);
			throw Error(_("Failed to initialise USB"));
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

// The following code is based on the example from the vsnprintf(3) manpage
std::string string_vprintf(const char *fmt, ...)
{
	// Guess we need no more than 200 bytes.
	int n, size = 200;
	char *p, *np;
	std::string result;
	va_list ap;

	if ((p = (char*)malloc(size)) == NULL)
		return NULL;

	for (;;) {
		// Try to print in the allocated space.
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);

		// If that worked, return the string.
		if (n > -1 && n < size) {
			result = p;
			free(p);
			return result;
		}

		// Else try again with more space.
		if (n > -1)    // glibc 2.1
			size = n+1; // precisely what is needed
		else           // glibc 2.0
			size *= 2;  // twice the old size

		if ((np = (char*)realloc (p, size)) == NULL) {
			free(p);
			return result;
		} else {
			p = np;
		}
	}
}

} // namespace Barry

