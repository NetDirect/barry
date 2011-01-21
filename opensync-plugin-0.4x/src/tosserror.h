///
/// \file	tosserror.h
///		RAII class for OSyncError structs that are wholly
///		belonging to the barry plugin.
///

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_TOSSERROR_H__
#define __BARRY_SYNC_TOSSERROR_H__

#include "trace.h"
#include <opensync/opensync.h>

//
// TossError
//
/// This is a wrapper class for OSyncError *error pointers.
/// For some of the opensync API, these error pointers are passed
/// into functions, such as osync_time_*() functions, and filled
/// in the event that an error occurs.  These kinds of *errors are
/// not passed into the framework, and so it is our responsibility
/// to free them.
///
/// This class makes it easy to do that freeing.
///
class TossError
{
	OSyncError *m_error;

	const char *m_func;
	Trace *m_trace;

public:
	// simple wrapper... unref's the error on destruction
	TossError()
		: m_error(0)
		, m_func(0)
		, m_trace(0)
	{
	}

	// log wrapper... logs to given tracer and unref's on destruction
	TossError(const char *funcname, Trace &trace)
		: m_error(0)
		, m_func(funcname)
		, m_trace(&trace)
	{
	}

	~TossError()
	{
		Clear();
	}

	/// Returns NULL if no error
	const char* GetErrorMsg()
	{
		return osync_error_print(&m_error);
	}

	bool IsSet()
	{
		return osync_error_is_set(&m_error);
	}

	void Log()
	{
		if( m_error && m_trace )
			m_trace->logf("%s: %s", m_func, GetErrorMsg());
	}

	void Clear()
	{
		if( m_error ) {
			Log();
			osync_error_unref(&m_error);
			m_error = 0;
		}
	}

	operator OSyncError**()
	{
		return &m_error;
	}
};

#endif

