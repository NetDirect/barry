///
/// \file	log.cc
///		General Barry interface routines
///

/*
    Copyright (C) 2008-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "log.h"
#include "clog.h"
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>

namespace Barry {

extern bool __data_dump_mode__;
extern std::ostream *LogStream;
extern pthread_mutex_t LogStreamMutex;

LogLock::LogLock()
{
	while( pthread_mutex_lock(&LogStreamMutex) != 0 )
		;
}

LogLock::~LogLock()
{
	pthread_mutex_unlock(&LogStreamMutex);
}


bool LogVerbose()
{
	return __data_dump_mode__;
}

std::ostream* GetLogStream()
{
	return LogStream;
}

} // namespace Barry

// Callable from C:

void BarryLogf(int verbose, const char *msg, ...)
{
	va_list vl;
	va_start(vl, msg);
	char buffer[2048];
	int n = vsnprintf(buffer, sizeof(buffer), msg, vl);
	va_end(vl);
	if( n > -1 && n < (int)sizeof(buffer) )
		strcpy(buffer, "BarryLog: (trace error, output too long for buffer)");

	if( verbose ) {
		barryverbose(buffer);
	}
	else {
		barrylog(buffer);
	}
}

