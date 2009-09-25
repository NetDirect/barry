///
/// \file	null-os40.cc
///		Null wrapper class for with opensync 0.4x is not available
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

#include "os40.h"

/////////////////////////////////////////////////////////////////////////////
// OpenSync40 - public members

OpenSync40::OpenSync40()
{
	throw std::runtime_error("OpenSync 0.4x support was not compiled in.");
}

OpenSync40::~OpenSync40()
{
}

/////////////////////////////////////////////////////////////////////////////
// Null implementations

const char* OpenSync40::GetVersion() const
{
	return 0;
}

void OpenSync40::GetPluginNames(string_list_type &plugins)
{
}

