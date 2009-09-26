///
/// \file	osbase.h
///		Base API class for OpenSync interaction.
///		This API will operate both 0.22 and 0.4x
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

#ifndef __BARRYDESKTOP_OSBASE_H__
#define __BARRYDESKTOP_OSBASE_H__

#include <vector>
#include <string>
#include <iosfwd>

typedef std::vector<std::string>		string_list_type;

std::ostream& operator<< (std::ostream &os, const string_list_type &list);

class OpenSyncAPI
{
public:
	OpenSyncAPI()
	{
	}

	virtual ~OpenSyncAPI()
	{
	}

	virtual const char* GetVersion() const = 0;
	virtual void GetPluginNames(string_list_type &plugins) = 0;
	virtual void GetGroupNames(string_list_type &groups) = 0;
};

#endif

