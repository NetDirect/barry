///
/// \file	error.h
///		Exception classes used specifically by the plugin.
///

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

#ifndef __BARRY_SYNC_ERROR_H__
#define __BARRY_SYNC_ERROR_H__

#include <stdexcept>
#include <opensync/opensync.h>

//////////////////////////////////////////////////////////////////////////////
// osync_error exception class

/// Exception class for wrapping OSyncError messages.  Takes ownership
/// of the error, and unref's it on destruction.
class osync_error : public std::runtime_error
{
	OSyncError *m_error;

public:
	explicit osync_error(OSyncError *error)
		: std::runtime_error(get_error_msg(error)),
		m_error(error)
	{
	}

	// For osync errors that have no error object
	explicit osync_error(const char *msg)
		: std::runtime_error(msg),
		m_error(0)
	{
	}

	// handle copying correctly
	osync_error(const osync_error &other)
		: std::runtime_error(other.what()),
		m_error(0)
	{
		operator=(other);
	}

	osync_error& operator=(const osync_error &other);
	~osync_error() throw();

	OSyncError* c_obj() { return m_error; }

	static std::string get_error_msg(OSyncError *error);
};


//////////////////////////////////////////////////////////////////////////////
// ConvertError exception class

class ConvertError : public std::runtime_error
{
public:
	ConvertError(const std::string &msg) : std::runtime_error(msg) {}
};


#endif

