///
/// \file	brawchannel.h
///		Directs a named raw channel over STDIN/STDOUT or TCP
///

/*
    Copyright (C) 2010-2012, RealVNC Ltd.

        Some parts are inspired from bjavaloader.cc

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

#ifndef __BARRY_TOOLS_BRAWCHANNEL_H__
#define __BARRY_TOOLS_BRAWCHANNEL_H__

#include <sys/types.h>
#include <memory>
#include <string>

// How long, in seconds, to wait between reads before checking if should shutdown
#define READ_TIMEOUT_SECONDS 1

/* Defined generic stream reading and writing classes.
 *
 * It'd be great to use iostream, but they don't provide non-blocking reads.
 */
class Stream {
public:
	virtual ~Stream() {};
};

class InputStream {
public:
	virtual ~InputStream() {};
	virtual ssize_t read(unsigned char* ptr, size_t size, int timeout) = 0;
};

class OutputStream {
public:
	virtual ~OutputStream() {};
	virtual ssize_t write(const unsigned char* ptr, size_t size) = 0;
};

class StdOutStream : public OutputStream {
public:
	virtual ssize_t write(const unsigned char* ptr, size_t size);
};

class StdInStream : public InputStream {
public:
	virtual ssize_t read(unsigned char* ptr, size_t size, int timeout);
};

// Forward declaration of platform implementation details
struct TcpStreamImpl;

class TcpStream : public Stream {
public:
	std::auto_ptr<TcpStreamImpl> mImpl;
public:
	TcpStream(const std::string& addr, long port);
	~TcpStream();
	bool accept();
};

class TcpInStream : public InputStream {
private:
	TcpStream& mStream;
public:
	TcpInStream(TcpStream& stream)
		: mStream(stream) {}
	virtual ssize_t read(unsigned char* ptr, size_t size, int timeout);
};

class TcpOutStream : public OutputStream {
private:
	TcpStream& mStream;
public:
	TcpOutStream(TcpStream& stream)
		: mStream(stream) {}
public:
	virtual ssize_t write(const unsigned char* ptr, size_t size);
};

#endif // __BARRY_TOOLS_BRAWCHANNEL_H__
