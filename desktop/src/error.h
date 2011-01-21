///
/// \file	error.h
///		Exception classes specific to the desktop
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

#ifndef __BARRYDESKTOP_ERROR_H__
#define __BARRYDESKTOP_ERROR_H__

#include <stdexcept>
#include <string>

/// \addtogroup exceptions
/// @{

//
// DlError
//
/// Represents and stores the error message from the last
/// dlopen() related error.
///
class DlError : public std::runtime_error
{
	static std::string GetMsg(const std::string &msg);

public:
	DlError(const std::string &msg);
};

/// @}

#endif

