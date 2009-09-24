///
/// \file	os40.cc
///		Wrapper class for opensync 0.22 syncing behaviour
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

//#include <../libopensync1/opensync/opensync.h>
//#include <../libopensync1/osengine/engine.h>

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// OpenSync40 - public members

OpenSync40::OpenSync40()
{
	if( !Open("libopensync.so.1") )
		throw DlError("Can't dlopen libopensync.so.1");

	// load all required symbols
	LoadSym(osync_get_version, "osync_get_version");
}

