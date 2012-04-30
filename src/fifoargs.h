///
/// \file	fifoargs.h
///		Class for passing command line arguments via fifo instead
///		of command line.
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_FIFOARGS_H__
#define __BARRY_FIFOARGS_H__

#include "dll.h"
#include "pin.h"
#include <iosfwd>

namespace Barry {

//
// FifoArgs
//
/// Contains argument variables to be passed through the FIFO.
/// It is no coincidence that there is a close connection to this
/// set of arguments and the arguments given to the pppob program.
///
struct BXEXPORT FifoArgs
{
	Pin m_pin;
	std::string m_password;
	std::string m_log_filename;
	bool m_use_serial_mode;
	bool m_verbose;

	FifoArgs()
		: m_use_serial_mode(false)
		, m_verbose(false)
	{
	}

	std::ostream& Write(std::ostream &os) const;
	std::istream& Read(std::istream &is);

	void Clear();
};

//
// FifoServer
//
/// Accepts a FifoArgs struct, and creates the necessary fifo for transfer.
/// To use, create the object, then execute the program (eg. pppob), and
/// then call Serve() with a given timeout in seconds.
///
/// This class deletes the fifo in the destructor, or explicitly, with
/// the Cleanup() call.
///
/// Only arguments that are valid are sent.
///
class BXEXPORT FifoServer
{
	const FifoArgs &m_args;
	bool m_created;

public:
	explicit FifoServer(const FifoArgs &args);
	~FifoServer();

	/// Serves the given arguments through the fifo.  Returns
	/// false on timeout.
	bool Serve(int timeout_sec);

	/// Deletes the fifo.  Called automatically by destructor.
	void Cleanup();
};

//
// FifoClient
//
/// Searches for a fifo and opens and reads it if available.  Use
/// Fetch() with a given timeout to perform the read attempt.
/// Use GetArgs() to access the filled FifoArgs struct.
///
class BXEXPORT FifoClient
{
	FifoArgs m_args;

public:
	FifoClient();

	/// Tries to open the fifo and read the arguments from it.
	/// If it fails in any way, or timeout, returns false.
	bool Fetch(int timeout_sec);

	const FifoArgs& GetArgs() const { return m_args; }
};

} // Barry namespace

#endif

