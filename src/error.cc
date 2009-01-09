///
/// \file	error.cc
///		Common exception classes for the Barry library
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <string.h>

using namespace std;

namespace Barry {

std::string BadSize::GetMsg(unsigned int p, unsigned int d, unsigned int r)
{
	std::ostringstream oss;
	oss << "Bad packet size. Packet: " << p
	    << ". DataSize(): " << d
	    << ". Required size: " << r;
	return oss.str();
}

std::string ErrnoError::GetMsg(const std::string &msg, int err)
{
	std::ostringstream oss;
	oss << msg << ": (errno " << err << ") " << strerror(err);
	return oss.str();
}

} // namespace Barry

