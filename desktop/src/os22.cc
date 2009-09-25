///
/// \file	os22.cc
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

#include "os22.h"
#include <barry/barry.h>
#include <memory>

//#include <../opensync-1.0/opensync/opensync.h>
//#include <../opensync-1.0/osengine/engine.h>

using namespace std;

class OpenSync22Private
{
public:
	// function pointers
	const char*		(*osync_get_version)();
};

/////////////////////////////////////////////////////////////////////////////
// OpenSync22 - public members

OpenSync22::OpenSync22()
{
	if( !Open("libosengine.so.0") )
		throw DlError("Can't dlopen libosengine.so.0");

	// store locally in case of constructor exception in LoadSym
	std::auto_ptr<OpenSync22Private> p(new OpenSync22Private);

	// load all required symbols...
	// we don't need to use try/catch here, since the base
	// class destructor will clean up for us if LoadSym() throws
	LoadSym(p->osync_get_version, "osync_get_version");

	// this pointer is ours now
	m_priv = p.release();
}

OpenSync22::~OpenSync22()
{
	delete m_priv;
	m_priv = 0;
}

const char* OpenSync22::GetVersion() const
{
	return m_priv->osync_get_version();
}

void OpenSync22::GetPluginNames(string_list_type &plugins)
{
	barryverbose("FIXME: OpenSync22::GetPluginNames() not implemented");
}

