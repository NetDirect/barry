///
/// \file	error.h
///		Common exception classes for the Barry library
///

/*
    Copyright (C) 2005-2006, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_ERROR_H__
#define __BARRY_ERROR_H__

#include <stdexcept>

namespace Barry {

/// \addtogroup exceptions
/// @{

//
// BError class
//
/// Currently the only Barry exception class, and planned to be the base class
/// for any future derived exceptions.  Thrown on any protocol error.
///
class BError : public std::runtime_error
{
public:
	BError(const std::string &str) : std::runtime_error(str) {}
	BError(int libusb_errno, const std::string &str);
};

/// @}

} // namespace Barry

#endif

