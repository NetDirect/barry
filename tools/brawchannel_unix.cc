///
/// \file	brawchannel_unix.cc
///		Implements OS support for STDIN/STDOUT and TCP
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

#include "brawchannel.h"
#include "i18n.h"
#include <barry/barry.h>

#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define SD_SEND SHUT_WR
#define INVALID_SOCKET -1
#define INVALID_HANDLE -1

using namespace std;
using namespace Barry;

struct TcpStreamImpl
{
	in_addr mHostAddress;
	const char * mListenAddress;
	long mPort;
	int mListenSocket;
	int mSocket;
	int mLastError;
};


ssize_t StdOutStream::write(const unsigned char* ptr, size_t size)
{
	size_t written = fwrite(ptr, 1, size, stdout);
	if( written == 0 &&
		( ferror(stdout) != 0 || feof(stdout) != 0 ) ) {
		return -1;
	}
	return static_cast<ssize_t>(written);
}

ssize_t StdInStream::read(unsigned char* ptr, size_t size, int timeout)
{
	fd_set rfds;
	struct timeval tv;
	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);
	tv.tv_sec = READ_TIMEOUT_SECONDS;
	tv.tv_usec = 0;
	int ret = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
	if( ret < 0 ) {
		cerr << _("Select failed with errno: ") << errno << endl;
		return -1;
	} else if ( ret && FD_ISSET(STDIN_FILENO, &rfds) ) {
		return ::read(STDIN_FILENO, ptr, size);
	} else {
		return 0;
	}
}

TcpStream::TcpStream(const char * addr, long port)
{
	mImpl.reset(new TcpStreamImpl);
	mImpl->mListenAddress = addr;
	mImpl->mPort = port;
	mImpl->mSocket = INVALID_SOCKET;
	mImpl->mListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( mImpl->mListenSocket == INVALID_SOCKET ) {
		cerr << _("Failed to create listening socket: ") << 
			errno << endl;
	}
	if( mImpl->mListenAddress == NULL ) {
		mImpl->mHostAddress.s_addr = INADDR_ANY;
	} else {
		mImpl->mHostAddress.s_addr = inet_addr(mImpl->mListenAddress);
	}
}

TcpStream::~TcpStream()
{
	if( mImpl->mSocket != INVALID_SOCKET ) {
		shutdown(mImpl->mSocket, SD_SEND);
		close(mImpl->mSocket);
		mImpl->mSocket = INVALID_SOCKET;
	}
	if( mImpl->mListenSocket != INVALID_SOCKET ) {
		shutdown(mImpl->mListenSocket, SD_SEND);
		close(mImpl->mListenSocket);
		mImpl->mListenSocket = INVALID_SOCKET;
	}
}

bool TcpStream::accept()
{
	if( mImpl->mListenSocket == INVALID_SOCKET ||
		mImpl->mLastError != 0 || 
		mImpl->mHostAddress.s_addr == INADDR_NONE ) {
		return false;
	}
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr = mImpl->mHostAddress;
	serverAddr.sin_port = htons(static_cast<u_short>(mImpl->mPort));
	if( ::bind(mImpl->mListenSocket, (sockaddr*) & serverAddr, sizeof(serverAddr)) < 0 ) {
		cerr << _("Failed to bind to listening address") << endl;
		return false;
	}

	// Set the socket options
	int one = 1;
	if( setsockopt(mImpl->mListenSocket, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<const char *> (&one), sizeof(one)) < 0 ) {
		cerr << _("Failed to enable reuse of address") << endl;
		return false;
	}

	if( fcntl(mImpl->mListenSocket, F_SETFL,
			fcntl(mImpl->mListenSocket, F_GETFL, 0) | O_NONBLOCK) < 0 ) {
		cerr << _("Failed to set non-blocking listening socket") << endl;
		return false;
	}

	if( ::listen(mImpl->mListenSocket, 5) == INVALID_SOCKET ) {
		cerr << _("Failed to listen to listening address") << endl;
		return false;
	}

	struct sockaddr_in clientAddr;
	socklen_t len = sizeof(clientAddr);
	cout << string_vprintf(_("Listening for connection on %s:%d"),
			( mImpl->mListenAddress == NULL ? "*" : mImpl->mListenAddress ),
			mImpl->mPort) << endl;

	mImpl->mSocket = ::accept(mImpl->mListenSocket, (struct sockaddr*) &clientAddr, &len);
	shutdown(mImpl->mListenSocket, SD_SEND);
	close(mImpl->mListenSocket);
	mImpl->mListenSocket = INVALID_SOCKET;
	if( mImpl->mSocket == INVALID_SOCKET ) {
		cerr << _("Failed to accept on listening socket") << endl;
		return false;
	}

	if( setsockopt(mImpl->mSocket, IPPROTO_TCP, TCP_NODELAY,
		reinterpret_cast<const char *> (&one), sizeof(one)) < 0 ) {
		cerr << _("Failed to set no delay") << endl;
		return false;
	}

	return true;
}

ssize_t TcpInStream::read(unsigned char* ptr, size_t size, int timeout)
{
	fd_set rfds;
	struct timeval tv;
	FD_ZERO(&rfds);
	FD_SET(mStream.mImpl->mSocket, &rfds);
	tv.tv_sec = READ_TIMEOUT_SECONDS;
	tv.tv_usec = 0;
	int ret = select(mStream.mImpl->mSocket + 1, &rfds, NULL, NULL, &tv);
	if( ret < 0 ) {
		cerr << _("Select failed with errno: ") << errno << endl;
		return -1;
	} else if ( ret && FD_ISSET(mStream.mImpl->mSocket, &rfds) ) {
		return ::recv(mStream.mImpl->mSocket, reinterpret_cast<char *>(ptr), size, 0);
	} else {
		return 0;
	}
}

ssize_t TcpOutStream::write(const unsigned char* ptr, size_t size)
{
	return ::send(mStream.mImpl->mSocket, reinterpret_cast<const char*>(ptr), size, 0);
}

