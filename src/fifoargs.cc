///
/// \file	fifoargs.cc
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

#include "fifoargs.h"
#include "error.h"
#include "common.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// FifoArgs class

std::ostream& FifoArgs::Write(std::ostream &os) const
{
	if( m_pin.Valid() )
		os << "Pin " << m_pin.Str() << endl;
	if( m_password.size() )
		os << "Password " << m_password << endl;
	if( m_log_filename.size() )
		os << "LogFilename " << m_log_filename << endl;
	if( m_use_serial_mode )
		os << "UseSerialMode" << endl;
	if( m_verbose )
		os << "Verbose" << endl;
	
	return os;
}

std::istream& FifoArgs::Read(std::istream &is)
{
	string line, token, arg;

	// start fresh
	Clear();

	while( getline(is, line) ) {
		istringstream iss(line);
		iss >> token >> ws;

		if( token == "Pin" )
			iss >> m_pin;
		else if( token == "Password" )
			getline(iss, m_password);
		else if( token == "LogFilename" )
			getline(iss, m_log_filename);
		else if( token == "UseSerialMode" )
			m_use_serial_mode = true;
		else if( token == "Verbose" )
			m_verbose = true;
	}

	return is;
}

void FifoArgs::Clear()
{
	m_pin.Clear();
	m_password.clear();
	m_log_filename.clear();
	m_use_serial_mode = false;
	m_verbose = false;
}


//////////////////////////////////////////////////////////////////////////////
// FifoServer class

FifoServer::FifoServer(const FifoArgs &args)
	: m_args(args)
	, m_created(false)
{
	int m_fifo = mkfifo(BARRY_FIFO_NAME, 0660);
	if( m_fifo != 0 )
		throw ErrnoError("Cannot open Barry argument fifo", errno);
	m_created = true;
}

FifoServer::~FifoServer()
{
	Cleanup();
}

bool FifoServer::Serve(int timeout_sec)
{
	if( !m_created )
		return false;

	// man fifo(7) says that opening write-only in non-blocking mode
	// will fail until the other side opens for read.  So continue
	// to attempt opens until out of time.
	timeout_sec *= 4;
	while( timeout_sec-- ) {
		// attempt to open in non-blocking mode
		int fd = open(BARRY_FIFO_NAME, O_WRONLY | O_NONBLOCK);
		if( fd == -1 ) {
			usleep(250000);
			continue;
		}

		ostringstream oss;
		m_args.Write(oss);
		int written = write(fd, oss.str().data(), oss.str().size());
		close(fd);

		// only success if we wrote all the data
		return written == (int)oss.str().size();
	}

	// timeout
	return false;
}

void FifoServer::Cleanup()
{
	if( m_created ) {
		unlink(BARRY_FIFO_NAME);
		m_created = false;
	}
}


//////////////////////////////////////////////////////////////////////////////
// FifoClient class

FifoClient::FifoClient()
{
}

/// Tries to open the fifo and read the arguments from it.
/// If it fails in any way, or timeout, returns false.
bool FifoClient::Fetch(int timeout_sec)
{
	// See man fifo(7).  Should always succeed, as long as
	// the file exists and permissions allow.
	int fd = open(BARRY_FIFO_NAME, O_RDONLY | O_NONBLOCK);
	if( fd == -1 )
		return false;

	string sbuf;
	timeout_sec *= 4;
	while( timeout_sec-- ) {
		char buf[4096];
		int r = read(fd, buf, sizeof(buf));

		if( r == 0 ) {
			// only consider this the end of file if
			// we've already read something, otherwise we close
			// before the server has a chance to speak up
			if( sbuf.size() )
				break;
			else
				usleep(250000);
		}
		else if( r < 0 ) {
			usleep(250000);
			continue;
		}
		else {
			timeout_sec++;
			sbuf.append(buf, r);
		}
	}
	close(fd);

	// parse
	istringstream iss(sbuf);
	m_args.Read(iss);
	return true;
}

} // Barry namespace

