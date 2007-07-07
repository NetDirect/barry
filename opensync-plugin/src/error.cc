///
/// \file	error.cc
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

#include "error.h"
#include <opensync/opensync.h>

//////////////////////////////////////////////////////////////////////////////
// osync_error exception class

osync_error& osync_error::operator=(const osync_error &other)
{
	// unref the existing error
	if( m_error ) {
		osync_error_unref(&m_error);
		m_error = 0;
	}

	// copy over the new one
	m_error = other.m_error;

	// ref it
	if( m_error )
		osync_error_ref(&m_error);

	return *this;
}

osync_error::~osync_error() throw()
{
	if( m_error )
		osync_error_unref(&m_error);
}

std::string osync_error::get_error_msg(OSyncError *error)
{
	const char *msg = osync_error_print(&error);
	if( msg )
		return msg; // can't pass null into ctor
	else
		return "(no error message: osync_error_print() returned NULL)";
}

