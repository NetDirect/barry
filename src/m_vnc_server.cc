///
/// \file	m_vnc_server.cc
///		Mode class for the VNCServer mode
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

#include "m_vnc_server.h"
#include "data.h"
#include "protocol.h"
#include "protostructs.h"
#include "packet.h"
#include "endian.h"
#include "error.h"
#include "usbwrap.h"
#include "controller.h"
#include <stdexcept>
#include <sstream>

#include "debug.h"

namespace Barry { namespace Mode {

static void HandleReceivedDataCallback(void* ctx, Data* data)
{
    ((VNCServer*)ctx)->HandleReceivedData(*data);
}

///////////////////////////////////////////////////////////////////////////////
// VNCServer Mode class

VNCServer::VNCServer(Controller &con, VNCServerDataCallback& callback)
	: Mode(con, Controller::VNCServer),
      Callback(callback)
{
}

VNCServer::~VNCServer()
{
}

void VNCServer::OnOpen()
{
    std::cerr << "about to register interest\n";
    m_socket->RegisterInterest(HandleReceivedDataCallback, this);
    std::cerr << "registered interest\n";
}

///////////////////////////////////////////////////////////////////////////////
// public API

void VNCServer::Send(Data& data)
{
    m_socket->Send(data);
}

void VNCServer::HandleReceivedData(Data& data)
{
    Callback.DataReceived(data);
}

}} // namespace Barry::Mode

