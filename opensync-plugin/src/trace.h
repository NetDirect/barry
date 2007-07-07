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

class Trace
{
	const char *text, *tag;
	bool m_error;
	char m_buffer[2048];

private:
	void do_logf(OSyncTraceType type, const char *msg, ...);

public:
	explicit Trace(const char *t);
	Trace(const char *t, const char *tag);
	~Trace();

	const char* get_last_msg() const { return m_buffer; }

	void log(const char *t);
	void logf(const char *t, ...);
	void log(OSyncXMLField *field);

	void error(const char *t);
	void errorf(const char *t, ...);
};

#endif

