///
/// \file	m_raw_channel.cc
///		Mode class for a raw channel
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

#include "m_raw_channel.h"
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

static void HandleReceivedDataCallback(void* ctx, Data* data) {
	((RawChannel*)ctx)->HandleReceivedData(*data);
}

///////////////////////////////////////////////////////////////////////////////
// RawChannel Mode class

RawChannel::RawChannel(Controller &con, RawChannelDataCallback& callback)
	: Mode(con, Controller::RawChannel),
      Callback(callback)
{
}

RawChannel::~RawChannel()
{
}

void RawChannel::OnOpen()
{
	m_socket->RegisterInterest(HandleReceivedDataCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
// public API

void RawChannel::Send(Data& data)
{
	Data toReceive;
	size_t packetSize = HeaderSize + data.GetSize();
	if(packetSize > MaximumPacketSize)
		throw Barry::Error("RawChannel: send data size larger than MaximumPacketSize");
	Barry::Protocol::Packet* packet = (Barry::Protocol::Packet*)m_sendBuffer;
	packet->socket = htobs(m_socket->GetSocket());
	packet->size = htobs(packetSize);
	std::memcpy(&(m_sendBuffer[HeaderSize]), data.GetData(), data.GetSize());

	Data toSend(m_sendBuffer, packetSize);
	m_socket->PacketData(toSend, toReceive, 0); // timeout immediately
	if (toReceive.GetSize() != 0)
		HandleReceivedData(toReceive);
}

void RawChannel::HandleReceivedData(Data& data)
{
	// Remove packet headers
	Data partial(data.GetData() + HeaderSize, data.GetSize() - HeaderSize);
	Callback.DataReceived(partial);
}

}} // namespace Barry::Mode

