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
#include "semaphore.h"
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
	: Mode(con, Controller::RawChannel)
	, m_callback(callback)
	, m_sendBuffer(0)
	, m_zeroRegistered(false)
	, m_mutex_valid(false)
	, m_cv_valid(false)
	, m_semaphore(NULL)
{
	m_sendBuffer = new unsigned char[MAX_PACKET_SIZE];

	// Create the thread synchronization objects
	if( pthread_mutex_init(&m_mutex, NULL) ) {
		throw Barry::Error("Failed to create mutex");
	}
	m_mutex_valid = true;
	if( pthread_cond_init(&m_cv, NULL) ) {
		throw Barry::Error("Failed to create condvar");
	}
	m_cv_valid = true;
	m_semaphore = new semaphore(m_mutex, m_cv);
}

RawChannel::~RawChannel()
{
	UnregisterZeroSocketInterest();
	delete[] m_sendBuffer;

	if( m_mutex_valid ) {
		pthread_mutex_destroy(&m_mutex);
	}
	if( m_cv_valid ) {
		pthread_cond_destroy(&m_cv);
	}
	delete m_semaphore;
}

void RawChannel::OnOpen()
{
	// Enable sequence packets so that DataSendAck callback and close can be
	// implemented
	m_zeroRegistered = true;
	m_socket->HideSequencePacket(false);
	m_con.m_queue->RegisterInterest(0, HandleReceivedDataCallback, this);
	m_socket->RegisterInterest(HandleReceivedDataCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
// public API

void RawChannel::Send(Data& data, int timeout)
{
	size_t packetSize = RAW_HEADER_SIZE + data.GetSize();

	if( packetSize > MAX_PACKET_SIZE ) {
		throw Barry::Error("RawChannel: send data size larger than MaximumPacketSize");
	}
	
	// setup header and copy data in
	MAKE_PACKETPTR_BUF(packet, m_sendBuffer);
	packet->socket = htobs(m_socket->GetSocket());
	packet->size = htobs(packetSize);
	std::memcpy(&(m_sendBuffer[RAW_HEADER_SIZE]), data.GetData(), data.GetSize());

	Data toSend(m_sendBuffer, packetSize);
	m_socket->Send(toSend, timeout);
	m_semaphore->WaitForSignal();
}

size_t RawChannel::MaximumSendSize()
{
	return MAX_PACKET_SIZE - RAW_HEADER_SIZE;
}
		
void RawChannel::HandleReceivedData(Data& data)
{
	Protocol::CheckSize(data, MIN_PACKET_DATA_SIZE);
	MAKE_PACKETPTR_BUF(packet, data.GetData());

	if( packet->socket == 0 ) {
		switch( btohs(packet->command) )
		{
		case SB_COMMAND_SEQUENCE_HANDSHAKE:
			m_semaphore->Signal();
			break;
		case SB_COMMAND_CLOSE_SOCKET:
		case SB_COMMAND_REMOTE_CLOSE_SOCKET:
			// Stop listening to socket 0 messages
			// so that socket close work.
			UnregisterZeroSocketInterest();
			m_callback.ChannelClose();
			break;
		default:
			UnregisterZeroSocketInterest();
			m_callback.ChannelError("RawChannel: Got unexpected socket zero packet");
			break;
		}
	}
	else {
		// Should be a socket packet for us, so remove packet headers
		Data partial(data.GetData() + RAW_HEADER_SIZE, data.GetSize() - RAW_HEADER_SIZE);
		m_callback.DataReceived(partial);
	}
}

void RawChannel::UnregisterZeroSocketInterest()
{
	if( m_zeroRegistered ) {
		m_con.m_queue->UnregisterInterest(0);
		m_socket->HideSequencePacket(true);
		m_zeroRegistered = false;
	}
}

}} // namespace Barry::Mode
