///
/// \file	null-os22.cc
///		Null wrapper class for when opensync 0.22 is not available
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

#include "os22.h"

/////////////////////////////////////////////////////////////////////////////
// OpenSync22 - public members

bool OpenSync22::symbols_loaded = false;

OpenSync22::OpenSync22()
{
	throw std::runtime_error("OpenSync 0.22 support was not compiled in.");
}

OpenSync22::~OpenSync22()
{
}

/////////////////////////////////////////////////////////////////////////////
// Null implementations

const char* OpenSync22::GetVersion() const
{
	return 0;
}

void OpenSync22::GetPluginNames(string_list_type &plugins)
{
}

