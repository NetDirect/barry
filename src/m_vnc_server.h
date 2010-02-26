///
/// \file	m_vnc_server.h
///		Mode class for a VNC server
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

#ifndef __BARRY_M_VNC_SERVER_H__
#define __BARRY_M_VNC_SERVER_H__

#include "dll.h"
#include "m_mode_base.h"
#include "socket.h"

namespace Barry {

namespace Mode {

// Callback from the VNC redirection session.

class BXEXPORT VNCServerDataCallback
{
public:
    virtual void DataReceived(Data& data) = 0;
    virtual ~VNCServerDataCallback() {};
};

//
// VNC Server class
//
/// The main class for creating a VNC redirection session.
///
/// To use this class, use the following steps:
///
/// - Implement VNCServerDataCallback
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::VNCServer object, passing in the Controller
///		object during construction
///	- Call Open() to open database socket and finish constructing.
///	- Call GetData() to fetch data
///	- Call SendData() to send data
///
class BXEXPORT VNCServer : public Mode
{
public:
	VNCServer(Controller &con, VNCServerDataCallback& callback);
	~VNCServer();

	//////////////////////////////////
	// VNC Server mode - VNC specific

    // Send some data to the VNC server.
    void Send(Data& data);

    void HandleReceivedData(Data& data);

    void OnOpen();

private:
    VNCServerDataCallback& Callback;

};

}} // namespace Barry::Mode

#endif

