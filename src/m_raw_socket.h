///
/// \file	m_raw_socket.h
///		Mode class for a raw socket
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)
    Portions Copyright (C) 2010 RealVNC Ltd.

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

#ifndef __BARRY_M_RAW_SOCKET_H__
#define __BARRY_M_RAW_SOCKET_H__

#include "dll.h"
#include "m_mode_base.h"
#include "socket.h"

namespace Barry {

namespace Mode {

// Callback from the raw socket.

class BXEXPORT RawSocketDataCallback
{
public:
    virtual void DataReceived(Data& data) = 0;
    virtual ~RawSocketDataCallback() {};
};

#define RAW_SOCKET_HEADER_SIZE 4
#define RAW_SOCKET_MAXIMUM_PACKET_SIZE (1 << 16)
#define RAW_SOCKET_MAXIMUM_PACKET_CONTENTS_SIZE (RAW_SOCKET_MAXIMUM_PACKET_SIZE - RAW_SOCKET_HEADER_SIZE)

//
// Raw socket class
//
/// The main class for creating a raw socket session.
///
/// To use this class, use the following steps:
///
/// - Implement RawSocketDataCallback
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::RawSocket object, passing in the Controller
///		object during construction
///	- Call Open() to open database socket and finish constructing.
///	- Call GetData() to fetch data
///	- Call SendData() to send data
///
class BXEXPORT RawSocket : public Mode
{
public:
	RawSocket(Controller &con, RawSocketDataCallback& callback);
	~RawSocket();

	//////////////////////////////////
	// Raw Socket mode specific methods

    // Send some data on the raw socket
    // Will throw an error if data is longer than
    // RAW_SOCKET_MAXIMUM_PACKET_CONTENTS_SIZE
    void Send(Data& data);

    void HandleReceivedData(Data& data);

    void OnOpen();

private:
    RawSocketDataCallback& Callback;
    unsigned char m_sendBuffer[RAW_SOCKET_MAXIMUM_PACKET_SIZE];

};

}} // namespace Barry::Mode

#endif

