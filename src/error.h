///
/// \file	error.h
///		Common exception classes for the Barry library
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "dll.h"
#include <stdexcept>
#include <stdint.h>

namespace Barry {

/// \addtogroup exceptions
/// @{

//
// Error class
//
/// The base class for any future derived exceptions.
/// Can be thrown on any protocol error.
///
class BXEXPORT Error : public std::runtime_error
{
public:
	Error(const std::string &str) : std::runtime_error(str) {}
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
///		BARRY_MIN_PASSWORD_TRIES tries remaining.  In this case,
///		out_of_tries() will return true.
///
///
class BXEXPORT BadPassword : public Barry::Error
{
	int m_remaining_tries;
	bool m_out_of_tries;

public:
	BadPassword(const std::string &str, int remaining_tries,
		bool out_of_tries)
		: Barry::Error(str),
		m_remaining_tries(remaining_tries),
		m_out_of_tries(out_of_tries)
		{}
	int remaining_tries() const { return m_remaining_tries; }
	bool out_of_tries() const { return m_out_of_tries; }
};

//
// BadData
//
/// Thrown by record classes if their data is invalid and cannot be
/// uploaded to the Blackberry.
///
class BXEXPORT BadData : public Barry::Error
{
public:
	BadData(const std::string &str)
		: Barry::Error(str)
		{}
};

//
// BadSize
//
/// Unexpected packet size, or not enough data.
///
class BXEXPORT BadSize : public Barry::Error
{
	unsigned int m_packet_size,
		m_data_buf_size,
		m_required_size;

	BXLOCAL static std::string GetMsg(const char *msg, unsigned int d, unsigned int r);
	BXLOCAL static std::string GetMsg(unsigned int p, unsigned int d, unsigned int r);

public:
	BadSize(const char *msg, unsigned int data_size, unsigned int required_size)
		: Barry::Error(GetMsg(msg, data_size, required_size))
		, m_packet_size(0)
		, m_data_buf_size(data_size)
		, m_required_size(required_size)
		{}
	BadSize(unsigned int packet_size,
		unsigned int data_buf_size,
		unsigned int required_size)
		: Barry::Error(GetMsg(packet_size, data_buf_size, required_size))
		, m_packet_size(packet_size)
		, m_data_buf_size(data_buf_size)
		, m_required_size(required_size)
		{}
	unsigned int packet_size() const { return m_packet_size; }
	unsigned int data_buf_size() const { return m_data_buf_size; }
	unsigned int required_size() const { return m_required_size; }
};

//
// ErrnoError
//
/// System error that provides an errno error code.
///
class BXEXPORT ErrnoError : public Barry::Error
{
	int m_errno;

	static std::string GetMsg(const std::string &msg, int err);

protected:
	ErrnoError(const std::string &msg) // for derived classes
		: Barry::Error(msg)
		, m_errno(0)
		{}

public:
	ErrnoError(const std::string &msg, int err)
		: Barry::Error(GetMsg(msg, err))
		, m_errno(err)
		{}

	int error_code() const { return m_errno; }
};

//
// ConfigFileError
//
/// Thrown by the ConfigFile class when encountering a serious system
/// error while loading the global config file for a given PIN.
///
class ConfigFileError : public Barry::ErrnoError
{
public:
	ConfigFileError(const char *msg) : Barry::ErrnoError(msg) {}
	ConfigFileError(const char *msg, int err)
		: Barry::ErrnoError(msg, err)
		{}
};

//
// BadPackedFormat
//
/// Thrown by record classes that don't recognize a given packed format code.
/// This exception is mostly handled internally, but is published here
/// just in case it escapes.
///
class BXEXPORT BadPackedFormat : public Barry::Error
{
	uint8_t m_format;

public:
	BadPackedFormat(uint8_t format)
		: Barry::Error("Bad packed format - internal exception")
		, m_format(format)
		{}

	uint8_t format() const { return m_format; }
};

//
// BadPacket
//
/// Thrown by the socket class if a packet command's response indicates
/// an error.  Some commands may be able to recover inside the library,
/// so a special exception is used, that includes the response code.
///
class BXEXPORT BadPacket : public Barry::Error
{
	uint8_t m_response;

public:
	BadPacket(uint8_t response, const std::string &msg)
		: Barry::Error(msg)
		, m_response(response)
		{}

	uint8_t response() const { return m_response; }
};

//
// ConvertError
//
/// Thrown by the vformat related barrysync library classes.
///
class ConvertError : public Barry::Error
{
public:
	ConvertError(const std::string &msg) : Barry::Error(msg) {}
};

//
// UnroutableReadError
//
/// Thrown by SocketRoutingQueue when a read is too small to determine
/// the socket, so is unroutable.
///
class UnroutableReadError : public Barry::Error
{
	BXLOCAL static std::string GetMsg(unsigned int read_size,
					  unsigned int min_size);
public:
	UnroutableReadError(unsigned int read_size,
		unsigned int min_size)
		: Barry::Error(GetMsg(read_size, min_size))
		{}
};

/// @}

} // namespace Barry

#endif

