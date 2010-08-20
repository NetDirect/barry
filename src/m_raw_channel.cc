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
#include "protostructs.h"

#include "debug.h"

#define RAW_HEADER_SIZE 4

namespace Barry { namespace Mode {

static void HandleReceivedDataCallback(void* ctx, Data* data) {
	((RawChannel*)ctx)->HandleReceivedData(*data);
}

///////////////////////////////////////////////////////////////////////////////
// RawChannel Mode class

RawChannel::RawChannel(Controller &con, RawChannelDataCallback& callback)
	: Mode(con, Controller::RawChannel),
	  Callback(callback),
	  m_sendBuffer(0)
{
	m_sendBuffer = new unsigned char[MAX_PACKET_SIZE];
}

RawChannel::~RawChannel()
{
	delete[] m_sendBuffer;
}

void RawChannel::OnOpen()
{
	// Enable sequence packets so that DataSendAck callback can be
	// implemented
	m_socket->HideSequencePacket(false);
	m_con.m_queue->RegisterInterest(0, HandleReceivedDataCallback, this);
	m_socket->RegisterInterest(HandleReceivedDataCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
// public API

void RawChannel::Send(Data& data, int timeout)
{
	size_t packetSize = RAW_HEADER_SIZE + data.GetSize();
	if(packetSize > MAX_PACKET_SIZE)
		throw Barry::Error("RawChannel: send data size larger than MaximumPacketSize");

	
	// setup header and copy data in
	MAKE_PACKETPTR_BUF(packet, m_sendBuffer);
	packet->socket = htobs(m_socket->GetSocket());
	packet->size = htobs(packetSize);
	std::memcpy(&(m_sendBuffer[RAW_HEADER_SIZE]), data.GetData(), data.GetSize());

	Data toSend(m_sendBuffer, packetSize);
	m_socket->Send(toSend, timeout);
}

size_t RawChannel::MaximumSendSize()
{
	return MAX_PACKET_SIZE - RAW_HEADER_SIZE;
}
		
void RawChannel::HandleReceivedData(Data& data)
{
	Protocol::CheckSize(data, MIN_PACKET_DATA_SIZE);
	MAKE_PACKETPTR_BUF(packet, data.GetData());

	if (packet->socket == 0) {
		// check it is a sequence handshake
		if (packet->command == htobs(SB_COMMAND_SEQUENCE_HANDSHAKE))
		{
			Callback.DataSendAck();
		}
		else
		{
			throw Barry::Error("RawChannel: Got unexpected socket zero packet");
		}
	} else {
		// Should be a socket packet for us, so remove packet headers
		Data partial(data.GetData() + RAW_HEADER_SIZE, data.GetSize() - RAW_HEADER_SIZE);
		Callback.DataReceived(partial);
	}
}

}} // namespace Barry::Mode
