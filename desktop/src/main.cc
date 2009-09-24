///
/// \file	main.cc
///		Program entry point for the desktop gui
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

#include <iostream>
#include <stdexcept>
#include <string>
#include <dlfcn.h>
#include <string.h>

#include <../opensync-1.0/opensync/opensync.h>
#include <../opensync-1.0/osengine/engine.h>

using namespace std;

class DlError : public std::runtime_error
{
	static std::string GetMsg(const char *msg)
	{
		std::string ret = msg;
		char *derr = dlerror();
		if( derr == NULL ) {
			ret += ": (dlerror returned NULL)";
		}
		else {
			ret += ": ";
			ret += derr;
		}
		return ret;
	}

public:
	DlError(const char *msg)
		: std::runtime_error(DlError::GetMsg(msg))
	{
	}

	DlError(const std::string &msg)
		: std::runtime_error(DlError::GetMsg(msg.c_str()))
	{
	}
};

class DlOpen
{
	void *m_handle;

protected:
	bool Open(const char *libname)
	{
		Shutdown();
		m_handle = dlopen(libname, RTLD_LAZY | RTLD_GLOBAL);
		return m_handle != NULL;
	}

	void Shutdown()
	{
		if( m_handle ) {
			dlclose(m_handle);
			m_handle = 0;
		}
	}

	template <class FuncPtrT>
	void LoadSym(FuncPtrT &funcptr, const char *symbol)
	{
		// work around the warning: ISO C++ forbids casting
		// between pointer-to-function and pointer-to-object
		//funcptr = (FuncPtrT) dlsym(this->m_handle, symbol);

		void *fptr = dlsym(this->m_handle, symbol);
		if( fptr == NULL )
			throw DlError(string("Can't load ") + symbol);

		if( sizeof(fptr) != sizeof(FuncPtrT) )
			throw std::logic_error("Platform not supported: sizeof(void*) != sizeof(void (*)())");

		memcpy(&funcptr, &fptr, sizeof(void*));
	}

public:
	DlOpen()
		: m_handle(0)
	{
	}

	virtual ~DlOpen()
	{
		Shutdown();
	}
};

class OpenSync22 : public DlOpen
{
public:
	OpenSync22()
	{
		if( !Open("libosengine.so") )
			throw DlError("Can't dlopen libosengine.so");

		// load all required symbols
		LoadSym(osync_get_version, "osync_get_version");
	}

	// opensync 0.22 function pointers
	const char* (*osync_get_version)();
};


int main()
{
	OpenSync22 os22;

	cout << os22.osync_get_version() << endl;

	return 0;
}

