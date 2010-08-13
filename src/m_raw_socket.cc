///
/// \file	m_raw_socket.cc
///		Mode class for the raw socket
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

#include "m_raw_socket.h"
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
#include <cstring>

#include "debug.h"

namespace Barry { namespace Mode {

static void HandleReceivedDataCallback(void* ctx, Data* data)
{
    ((RawSocket*)ctx)->HandleReceivedData(*data);
}

///////////////////////////////////////////////////////////////////////////////
// RawSocket Mode class

RawSocket::RawSocket(Controller &con, RawSocketDataCallback& callback)
	: Mode(con, Controller::RawSocket),
      Callback(callback)
{
}

RawSocket::~RawSocket()
{
}

void RawSocket::OnOpen()
{
    m_socket->RegisterInterest(HandleReceivedDataCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
// public API

void RawSocket::Send(Data& data)
{
    Data toReceive;
    try
    {
        size_t packetSize = 4 + data.GetSize();
	Barry::Protocol::Packet* packet = (Barry::Protocol::Packet*)m_sendBuffer;
	packet->socket = htobs(m_socket->GetSocket());
	packet->size = htobs(packetSize);
	std::memcpy(&(m_sendBuffer[4]), data.GetData(), data.GetSize());

	Data toSend(m_sendBuffer, packetSize);
        m_socket->PacketData(toSend, toReceive, 0); // timeout immediately
        if (toReceive.GetSize() != 0)
            HandleReceivedData(toReceive);
    }
    catch (Usb::Error& err)
    {
    }
}

void RawSocket::HandleReceivedData(Data& data)
{
    // Remove packet headers
    Data partial(data.GetData() + 4, data.GetSize() - 4);
    Callback.DataReceived(partial);
}

}} // namespace Barry::Mode

