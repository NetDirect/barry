///
/// \file	osprivatebase.h
///		Base API class for OpenSync sync conflicts
///		This API will operate both 0.22 and 0.4x
///

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_OSPRIVATEBASE_H__
#define __BARRYDESKTOP_OSPRIVATEBASE_H__

#include <iosfwd>
#include <string>

namespace OpenSync {

class SyncConflictPrivateBase
{
public:
	virtual ~SyncConflictPrivateBase() {}

	virtual bool IsAbortSupported() const = 0;
	virtual bool IsIgnoreSupported() const = 0;
	virtual bool IsKeepNewerSupported() const = 0;

	virtual void Select(int change_id) = 0;
	virtual void Abort() = 0;
	virtual void Duplicate() = 0;
	virtual void Ignore() = 0;
	virtual void KeepNewer() = 0;
};

class SyncSummaryPrivateBase
{
public:
	virtual ~SyncSummaryPrivateBase() {}

	virtual void Abort() = 0;
	virtual void Continue() = 0;
};

}

#endif

