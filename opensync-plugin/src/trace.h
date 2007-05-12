//
// \file	trace.h
//		RAII class for trace logging.
//

/*
    Copyright (C) 2006-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_TRACE_H__
#define __BARRY_SYNC_TRACE_H__

#include <opensync/opensync.h>
#include <stdarg.h>

class Trace
{
	const char *text, *tag;
	bool m_error;
public:
	explicit Trace(const char *t) : text(t), tag(0), m_error(false)
	{
		osync_trace(TRACE_ENTRY, "barry_sync: %s", text);
	}

	Trace(const char *t, const char *tag) : text(t), tag(tag), m_error(false)
	{
		osync_trace(TRACE_ENTRY, "barry_sync (%s): %s", tag, text);
	}

	~Trace()
	{
		if( tag )
			osync_trace(m_error ? TRACE_EXIT_ERROR : TRACE_EXIT,
				"barry_sync (%s): %s", tag, text);
		else
			osync_trace(m_error ? TRACE_EXIT_ERROR : TRACE_EXIT,
				"barry_sync: %s", text);
	}

	void log(const char *t)
	{
		osync_trace(TRACE_INTERNAL, "barry_sync: %s", t);
	}

	void error(const char *t)
	{
		m_error = true;
		osync_trace(TRACE_ERROR, "barry_sync: %s", t);
	}

	void logf(const char *t, ...)
	{
		va_list vl;
		va_start(vl, t);
		char buffer[2048];
		int n = vsnprintf(buffer, sizeof(buffer), t, vl);
		va_end(vl);
		if( n > -1 && n < (int)sizeof(buffer) )
			osync_trace(TRACE_INTERNAL, "barry_sync: %s", buffer);
		else
			osync_trace(TRACE_INTERNAL, "barry_sync: (trace error, output too long for buffer: %s)", t);
	}

	void errorf(const char *t, ...)
	{
		m_error = true;

		va_list vl;
		va_start(vl, t);
		char buffer[2048];
		int n = vsnprintf(buffer, sizeof(buffer), t, vl);
		va_end(vl);
		if( n > -1 && n < (int)sizeof(buffer) )
			osync_trace(TRACE_ERROR, "barry_sync: %s", buffer);
		else
			osync_trace(TRACE_ERROR, "barry_sync: (trace error, output too long for buffer: %s)", t);
	}
};

#endif

