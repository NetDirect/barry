///
/// \file	modem.h
///		Modem API base class for the various serial/modem
///		modes available on the Blackberry.
///

/*
    Copyright (C) 2008-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_MODEM_H__
#define __BARRY_MODEM_H__

#include "dll.h"

namespace Barry {

class Data;

class BXEXPORT Modem
{
public:
	virtual ~Modem() {}

	virtual void Open(const char *password = 0) = 0;
	virtual void Close() = 0;
	virtual void Write(const Data &data, int timeout = -1) = 0;
};

}

#endif

