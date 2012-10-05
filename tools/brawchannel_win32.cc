///
/// \file	brawchannel_win32.cc
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

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#define INVALID_HANDLE ((HANDLE)NULL)
// This is a name of a public semaphore to signal when the listen socket is opened
#define LISTEN_SEMAPHORE_NAME _T("Barry_brawchannel_%s_%d_startup_rendezvous")
#define LISTEN_SEMAPHORE_MAX_LEN 255
#define LISTEN_ADDRESS_MAX 128

using namespace std;
using namespace Barry;

struct TcpStreamImpl
{
	in_addr mHostAddress;
	// FIXME - this should be a std::string if ever moved to the library
	const char * mListenAddress;
	long mPort;
	SOCKET mListenSocket;
	SOCKET mSocket;
	HANDLE mEvent;
	DWORD mLastError;
};

ssize_t StdOutStream::write(const unsigned char* ptr, size_t size)
{
	size_t written = fwrite(ptr, 1, size, stderr);
	if( written == 0 &&
		( ferror(stderr) != 0 || feof(stderr) != 0 ) ) {
		return -1;
	}
	return static_cast<ssize_t>(written);
}

/* Windows terminal input class implementation */
ssize_t StdInStream::read(unsigned char* ptr, size_t size, int timeout)
{
	/* Windows CE can't do non-blocking IO, so just always fail to read anything*/
	Sleep(timeout * 1000);
	return 0;
}

TcpStream::TcpStream(const char * addr, long port)
{
	mImpl.reset(new TcpStreamImpl);
	mImpl->mListenAddress = addr;
	mImpl->mPort = port;
	mImpl->mListenSocket = INVALID_SOCKET;
	mImpl->mSocket = INVALID_SOCKET;
	mImpl->mEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	WSADATA wsaData;
	mImpl->mLastError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if( mImpl->mLastError != 0 ) {
		cerr << _("Failed to startup WSA: ") << mImpl->mLastError << endl;
	}
	mImpl->mListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( mImpl->mListenSocket == INVALID_SOCKET ) {
		cerr << _("Failed to create listening socket: ") << 
			WSAGetLastError() << endl;
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
		closesocket(mImpl->mSocket);
		mImpl->mSocket = INVALID_SOCKET;
	}
	if( mImpl->mListenSocket != INVALID_SOCKET ) {
		shutdown(mImpl->mListenSocket, SD_SEND);
		closesocket(mImpl->mListenSocket);
		mImpl->mListenSocket = INVALID_SOCKET;
	}
	if( mImpl->mEvent != INVALID_HANDLE ) {
		CloseHandle(mImpl->mEvent);
		mImpl->mEvent = INVALID_HANDLE;
	}
	WSACleanup();
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

	ULONG longOne = 1;
	if( ioctlsocket(mImpl->mListenSocket, FIONBIO, &longOne) == INVALID_SOCKET ) {
		cerr << _("Failed to set non-blocking listening socket") << endl;
		return false;
	}

	if( ::listen(mImpl->mListenSocket, 5) == INVALID_SOCKET ) {
		cerr << _("Failed to listen to listening address") << endl;
		return false;
	}

	struct sockaddr_in clientAddr;
	socklen_t len = sizeof(clientAddr);
	cout << string_vprintf(_("Listening for connection on %s:%ld"),
			( mImpl->mListenAddress == NULL ? "*" : mImpl->mListenAddress ),
			mImpl->mPort) << endl;

	/* Signal to a public semaphore that the listen socket is up */
	TCHAR wListenAddress[LISTEN_ADDRESS_MAX];
	if( MultiByteToWideChar(CP_ACP, 0,
			(mImpl->mListenAddress == NULL ? "*" : mImpl->mListenAddress), -1,
			wListenAddress, LISTEN_ADDRESS_MAX) > 0 ) {
		TCHAR semName[LISTEN_SEMAPHORE_MAX_LEN];
		_snwprintf(semName, LISTEN_SEMAPHORE_MAX_LEN, LISTEN_SEMAPHORE_NAME, wListenAddress, mImpl->mPort);
		semName[LISTEN_SEMAPHORE_MAX_LEN - 1] = 0;
		HANDLE sem = CreateSemaphore(NULL, 0, 1, semName);
		if( sem != NULL ) {
			ReleaseSemaphore(sem, 1, NULL);
			CloseHandle(sem);
		}
	}

	int ret = WSAEventSelect(mImpl->mListenSocket, mImpl->mEvent, FD_ACCEPT);
	if( ret != 0 ) {
		cerr << _("WSAEventSelect failed with error: ") << ret << endl;
		return false;
	}
	DWORD signalledObj = WaitForSingleObject(mImpl->mEvent, INFINITE);
	if( signalledObj != WAIT_OBJECT_0 ) {
		cerr << _("Failed to wait for new connection: ") << signalledObj << endl;
		return false;
	}

	mImpl->mSocket = ::accept(mImpl->mListenSocket, (struct sockaddr*) &clientAddr, &len);
	shutdown(mImpl->mListenSocket, SD_SEND);
	closesocket(mImpl->mListenSocket);
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
	int ret = WSAEventSelect(mStream.mImpl->mSocket, mStream.mImpl->mEvent, FD_READ);
	if( ret != 0 ) {
		cerr << _("WSAEventSelect failed with error: ") << ret << endl;
		return -1;
	}
	switch( WaitForSingleObject(mStream.mImpl->mEvent, timeout * 1000) ) {
		case WAIT_ABANDONED:
		case WAIT_TIMEOUT:
			return 0;
		case WAIT_OBJECT_0:
			ResetEvent(mStream.mImpl->mEvent);
			ret = ::recv(mStream.mImpl->mSocket, reinterpret_cast<char *>(ptr), size, 0);
			if( ret == SOCKET_ERROR ) {
				int wsaErr = WSAGetLastError();
				switch( wsaErr ) {
				case WSAEWOULDBLOCK:
					return 0;
				default:
					return -1;
				}
			} else {
				return ret;
			}
		case WAIT_FAILED:
		default:
			cerr << _("WaitForSingleObject failed with error: ") << GetLastError() << endl;
			return -1;
	};
}


ssize_t TcpOutStream::write(const unsigned char* ptr, size_t size)
{
	return ::send(mStream.mImpl->mSocket, reinterpret_cast<const char*>(ptr), size, 0);
}

