///
/// \file	error.h
///		Common exception classes for the Barry library
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

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
// Error class
//
/// The base class for any future derived exceptions.
/// Can be thrown on any protocol error.
///
class Error : public std::runtime_error
{
public:
	Error(const std::string &str) : std::runtime_error(str) {}
	Error(int libusb_errno, const std::string &str);
};


//
// BadPassword
//
/// A bad or unknown password when talking to the device.
/// Can be thrown in the following instances:
///
///	- no password provided and the device requests one
///	- device rejected the available password
///	- too few remaining tries left... Barry will refuse to keep
///		trying passwords if there are fewer than
///		6 tries remaining
///		
///
class BadPassword : public Barry::Error
{
	int m_remaining_tries;

public:
	BadPassword(const std::string &str, int remaining_tries)
		: Barry::Error(str),
		m_remaining_tries(remaining_tries)
		{}
	int remaining_tries() const { return m_remaining_tries; }
};

/// @}

} // namespace Barry

#endif

