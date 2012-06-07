///
/// \file	error.cc
///		Common exception classes for the Barry library
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <errno.h>
#include <sstream>
#include <string.h>

using namespace std;

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// BadSize exception

BadSize::BadSize(const char *msg, unsigned int data_size,
			unsigned int required_size)
	: Barry::Error(GetMsg(msg, data_size, required_size))
	, m_packet_size(0)
	, m_data_buf_size(data_size)
	, m_required_size(required_size)
{
}

BadSize::BadSize(unsigned int packet_size,
			unsigned int data_buf_size,
			unsigned int required_size)
	: Barry::Error(GetMsg(packet_size, data_buf_size, required_size))
	, m_packet_size(packet_size)
	, m_data_buf_size(data_buf_size)
	, m_required_size(required_size)
{
}

std::string BadSize::GetMsg(const char *msg, unsigned int d, unsigned int r)
{
	std::ostringstream oss;
	oss << msg << ": Bad packet size, not enough data: DataSize(): " << d
	    << ". Required size: " << r;
	return oss.str();
}

std::string BadSize::GetMsg(unsigned int p, unsigned int d, unsigned int r)
{
	std::ostringstream oss;
	oss << "Bad packet size. Packet: " << p
	    << ". DataSize(): " << d
	    << ". Required size: " << r;
	return oss.str();
}


//////////////////////////////////////////////////////////////////////////////
// ErrnoError exception

ErrnoError::ErrnoError(const std::string &msg)
	: Barry::Error(msg)
	, m_errno(0)
{
}

ErrnoError::ErrnoError(const std::string &msg, int err)
	: Barry::Error(GetMsg(msg, err))
	, m_errno(err)
{
}

std::string ErrnoError::GetMsg(const std::string &msg, int err)
{
	std::ostringstream oss;
	oss << msg << ": (errno " << err << ") " << strerror(err);
	return oss.str();
}

} // namespace Barry

