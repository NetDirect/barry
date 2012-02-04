///
/// \file	boostwrap.h
///		Wrap Boost serialization into its own lib, so each usage
///		in the tools doesn't cause a compile speed meltdown.
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_TOOLS_BOOSTWRAP_H__
#define __BARRY_TOOLS_BOOSTWRAP_H__

#include <barry/barry.h>

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
bool LoadBoostFile(const std::string &filename, \
	std::vector<Barry::tname> &container, \
	std::string &dbName, \
	std::string &errmsg); \
bool SaveBoostFile(const std::string &filename, \
	const std::vector<Barry::tname> &container, \
	std::string &errmsg);
ALL_KNOWN_PARSER_TYPES

#endif

