///
/// \file	error.cc
///		Exception class implementations
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <dlfcn.h>

/////////////////////////////////////////////////////////////////////////////
// class DlError

std::string DlError::GetMsg(const std::string &msg)
{
	std::string ret = msg;
	char *derr = dlerror();
	if( derr == NULL ) {
		ret += ": (dlerror returned NULL)";
	}
	else {
		ret += ": ";
		ret += derr;
	}
	return ret;
}

DlError::DlError(const std::string &msg)
	: std::runtime_error(DlError::GetMsg(msg.c_str()))
{
}

