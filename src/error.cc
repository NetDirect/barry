///
/// \file	error.cc
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

#include "error.h"
#include <sstream>
#include <iomanip>

using namespace std;

namespace Barry {

std::string GetErrorString(int libusb_errno, const std::string &str)
{
	ostringstream oss;
	oss << "(" << setbase(10) << libusb_errno << ", "
	    << strerror(-libusb_errno) << "): "
	    << str;
	return oss.str();
}

Error::Error(int libusb_errno, const std::string &str)
	: std::runtime_error(GetErrorString(libusb_errno, str))
{
}

} // namespace Barry

