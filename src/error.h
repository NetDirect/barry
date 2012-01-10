///
/// \file	error.h
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

#ifndef __BARRY_ERROR_H__
#define __BARRY_ERROR_H__

#include "dll.h"
#include "pin.h"
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
// SocketCloseOnOpen
//
/// Thrown by the Socket class if it receives a CLOSE message in
/// response to an OPEN command.  This can mean a number of things:
///
///	- device is password protected, and the wrong password was given
///	- device thinks that the socket is already open
///
/// This special exception thrown so the application can try again
/// with a fresh Socket::Open() call.
///
class BXEXPORT SocketCloseOnOpen : public Barry::Error
{
public:
	SocketCloseOnOpen(const std::string &str) : Barry::Error(str) {}
};

//
// PinNotFound
//
/// Thrown by the Connector class when unable to find the requested Pin
/// If the attached pin is not Valid(), then unable to autodetect device.
/// If pin is Valid(), then the specified pin number was not available.
/// probe_count is the number of devices found during the probe.
///
class BXEXPORT PinNotFound : public Barry::Error
{
	Barry::Pin m_pin;
	int m_probe_count;

public:
	PinNotFound(Barry::Pin pin, int probe_count)
		: Barry::Error("PIN not found: " + pin.Str())
		, m_pin(pin)
		, m_probe_count(probe_count)
		{}

	const Barry::Pin& pin() const { return m_pin; }
	int probe_count() const { return m_probe_count; }
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
	BadSize(const char *msg, unsigned int data_size, unsigned int required_size);
	BadSize(unsigned int packet_size,
		unsigned int data_buf_size,
		unsigned int required_size);
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

	BXLOCAL static std::string GetMsg(const std::string &msg, int err);

protected:
	ErrnoError(const std::string &msg); // for derived classes

public:
	ErrnoError(const std::string &msg, int err);

	int error_code() const { return m_errno; }
};

//
// ConfigFileError
//
/// Thrown by the ConfigFile class when encountering a serious system
/// error while loading the global config file for a given PIN.
///
class BXEXPORT ConfigFileError : public Barry::ErrnoError
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
class BXEXPORT ConvertError : public Barry::Error
{
public:
	ConvertError(const std::string &msg) : Barry::Error(msg) {}
};

//
// BackupError
//
/// Thrown by the Backup parser class when there is a problem with the
/// low level file operation.
///
class BXEXPORT BackupError : public Barry::Error
{
public:
	BackupError(const std::string &str) : Error(str) {}
};

//
// RestoreError
//
/// Thrown by the Restore builder class when there is a problem with the
/// low level file operation.
///
class BXEXPORT RestoreError : public Barry::Error
{
public:
	RestoreError(const std::string &str) : Error(str) {}
};

//
// ReturnCodeError
//
/// Thrown by the Mode::Desktop class when a database command returns
/// a non-zero error code.  Can happen when writing or clearing a database.
/// The packet command and return code are passed along, for examination
/// by application code.  Note that return code 0x02 usually means
/// you're trying to clear or write to a read-only database, like Time Zones.
///
class BXEXPORT ReturnCodeError : public Barry::Error
{
	unsigned int m_command, m_return_code;

public:
	ReturnCodeError(const std::string &str, unsigned int command,
			unsigned int return_code)
		: Error(str)
		, m_command(command)
		, m_return_code(return_code)
	{
	}

	unsigned int command() const { return m_command; }
	unsigned int return_code() const { return m_return_code; }
};

/// @}

} // namespace Barry

#endif

