///
/// \file	jdwp.cc
///		JDWP socket communication implementation
///

/*
    Copyright (C) 2009, Nicolas VIVIEN

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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <barry/barry.h>

#include "jdwp.h"


namespace JDWP {


///////////////////////////////////////////////////////////////////////////////
// JDWP::Error exception class

static std::string GetErrorString(int errcode, const std::string &str)
{
	std::ostringstream oss;
	oss << "(";

	if( errcode ) {
		oss << std::setbase(10) << errcode << ", ";
	}

//	oss << strerror(-libusb_errno) << "): "
	oss << "): ";
	oss << str;
	return oss.str();
}


Error::Error(const std::string &str)
	: Barry::Error(GetErrorString(0, str))
	, m_errcode(0)
{
}


Error::Error(int errcode, const std::string &str)
	: Barry::Error(GetErrorString(errcode, str))
	, m_errcode(errcode)
{
}



///////////////////////////////////////////////////////////////////////////////
// JDWP::JDWP communication class

JDWP::JDWP()
{
}


JDWP::~JDWP()
{
}


bool JDWP::Read(int socket, Barry::Data &data, int timeout)
{
	int ret;

	do {
		data.QuickZap();

		ret = read(socket, (char*) data.GetBuffer(), data.GetBufSize());

		if( ret < 0 ) {
			ret = -errno;
			if (ret != -EINTR && ret != -EAGAIN ) {
				m_lasterror = ret;
				if( ret == -ETIMEDOUT )
					throw Timeout(ret, "Timeout in read");
				else
					throw Error(ret, "Error in read");
			}
		}
		else
			data.ReleaseBuffer(ret);
	} while( ret == -EINTR );

	return ret >= 0;
}


bool JDWP::Write(int socket, const Barry::Data &data, int timeout)
{
	int ret;

	do {
		ret = write(socket,	(char*) data.GetData(), data.GetSize());

		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in write (1)");
			else
				throw Error(ret, "Error in write (1)");
		}
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}


bool JDWP::Write(int socket, const void *data, size_t size, int timeout)
{
	int ret;

	do {
		ret = write(socket, (char*) data, size);

		if( ret < 0 && ret != -EINTR && ret != -EAGAIN ) {
			m_lasterror = ret;
			if( ret == -ETIMEDOUT )
				throw Timeout(ret, "Timeout in write (2)");
			else
				throw Error(ret, "Error in write (2)");
		}
	} while( ret == -EINTR || ret == -EAGAIN );

	return ret >= 0;
}

} // namespace JDWP

