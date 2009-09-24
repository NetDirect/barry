///
/// \file	os40.h
///		Wrapper class for opensync 0.4x syncing behaviour
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

#ifndef __BARRYDESKTOP_OS40_H__
#define __BARRYDESKTOP_OS40_H__

#include "dlopen.h"

class OpenSync40 : public DlOpen
{
public:
	OpenSync40();

	// opensync 0.40 function pointers
	const char* (*osync_get_version)();
};

#endif

