///
/// \file	version.cc
///		Provide access to library version information
///

/*
    Copyright (C) 2007-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "version.h"
#include "config.h"

#ifdef WORDS_BIGENDIAN
#define BARRY_VERSION_STRING	"Barry library version " BARRY_VER_STRING " (big endian)"
#else
#define BARRY_VERSION_STRING	"Barry library version " BARRY_VER_STRING " (little endian)"
#endif

#define BARRY_VERSION_LOGICAL	BARRY_LOGICAL
#define BARRY_VERSION_MAJOR	BARRY_MAJOR
#define BARRY_VERSION_MINOR	BARRY_MINOR

namespace Barry {

/// Fills major and minor with integer version numbers, and
/// returns a string containing human readable version
/// information in English.
const char* Version(int &logical, int &major, int &minor)
{
	major = BARRY_VERSION_MAJOR;
	minor = BARRY_VERSION_MINOR;
	return BARRY_VERSION_STRING;
}

} // namespace Barry

