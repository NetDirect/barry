///
/// \file	dlopen.h
///		Base class wrapper for dlopen() behaviour
///		Drawback: only one dlopen'd library per class.
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

#ifndef __BARRYDESKTOP_DLOPEN_H__
#define __BARRYDESKTOP_DLOPEN_H__

#include "error.h"
#include <dlfcn.h>
#include <string.h>
#include <string>

//
// DlOpen
//
/// Base class wrapper for dlopen() libraries.  Only handles one
/// library handle at a time.
///
class DlOpen
{
	void *m_handle;

protected:
	bool Open(const char *libname);
	void Shutdown();

	template <class FuncPtrT>
	void LoadSym(FuncPtrT &funcptr, const char *symbol)
	{
		// work around the warning: ISO C++ forbids casting
		// between pointer-to-function and pointer-to-object
		//funcptr = (FuncPtrT) dlsym(this->m_handle, symbol);

		void *fptr = dlsym(this->m_handle, symbol);
		if( fptr == NULL )
			throw DlError(std::string("Can't load ") + symbol);

		if( sizeof(fptr) != sizeof(FuncPtrT) )
			throw std::logic_error("Platform not supported: sizeof(void*) != sizeof(void (*)())");

		memcpy(&funcptr, &fptr, sizeof(void*));
	}

public:
	DlOpen();
	virtual ~DlOpen();
};

#endif

