///
/// \file	util.h
///		GUI utility functions
///

/*
    Copyright (C) 2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_UTIL_H__
#define __BARRYBACKUP_UTIL_H__

#include <libglademm.h>
#include <string>

Glib::RefPtr<Gnome::Glade::Xml> LoadXml(const char *filename);
std::string GetPath(const std::string &filename);
bool CheckPath(const std::string &path, std::string *perr = 0);

#endif

