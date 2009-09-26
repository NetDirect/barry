///
/// \file	util.cc
///		Misc utility functions
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

#include "util.h"
#include <pwd.h>

std::string GetHomeDir()
{
	uid_t me = getuid();

	struct passwd p, *pp;
	char buf[1024];
	if( getpwuid_r(me, &p, buf, sizeof(buf), &pp) != 0 ) {
		return "/tmp";
	}

	return p.pw_dir;
}

