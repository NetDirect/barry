///
/// \file	dlopen.cc
///		Base class wrapper for dlopen() behaviour
///

/*
    Copyright (C) 2009-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "dlopen.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <dlfcn.h>
#include <string.h>

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// DlOpen class -- protected members

bool DlOpen::Open(const char *libname)
{
	Shutdown();
	m_handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
	return m_handle != NULL;
}

void DlOpen::Shutdown()
{
	if( m_handle ) {
		if( dlclose(m_handle) != 0 ) {
			cout << "ERROR: dlclose() return non-zero" << endl;
			cout << dlerror() << endl;
		}
		m_handle = 0;
	}
}


/////////////////////////////////////////////////////////////////////////////
// DlOpen class -- public members

DlOpen::DlOpen()
	: m_handle(0)
{
}

DlOpen::~DlOpen()
{
	Shutdown();
}

