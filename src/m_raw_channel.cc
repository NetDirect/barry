///
/// \file	m_raw_channel.cc
///		Mode class for a raw channel
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)
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
#include <string>
#include "protostructs.h"

#include "debug.h"

namespace Barry { namespace Mode {

///////////////////////////////////////////////////////////////////////////////
// RawChannel SocketDataHandler callback class for data socket
class RawChannelSocketHandler: public SocketRoutingQueue::SocketDataHandler
{
	RawChannel &m_raw_channel;
public:
	RawChannelSocketHandler(RawChannel &raw_channel)
		: m_raw_channel(raw_channel)
	{}
	virtual void DataReceived(Data &data)
	{
		m_raw_channel.HandleReceivedData(data);
	}
	virtual void Error(Barry::Error &error)
	{
		SocketDataHandler::Error(error);
		m_raw_channel.HandleError(error);
	}
	virtual ~RawChannelSocketHandler()
	{}
};

///////////////////////////////////////////////////////////////////////////////
// RawChannel SocketDataHandler callback class for zero socket
class RawChannelZeroSocketHandler: public SocketRoutingQueue::SocketDataHandler
{
	RawChannel &m_raw_channel;
public:
	RawChannelZeroSocketHandler(RawChannel &raw_channel)
		: m_raw_channel(raw_channel)
	{}
	virtual void DataReceived(Data &data)
	{
		m_raw_channel.HandleReceivedZeroPacket(data);
	}
	virtual void Error(Barry::Error &error)
	{
		SocketDataHandler::Error(error);
		m_raw_channel.HandleError(error);
	}
	virtual ~RawChannelZeroSocketHandler()
	{}
};

///////////////////////////////////////////////////////////////////////////////
// RawChannel Mode class

RawChannel::RawChannel(Controller &con, RawChannelDataCallback &callback)
	: Mode(con, Controller::RawChannel)
	, m_mutex_valid(false)
	, m_cv_valid(false)
	, m_semaphore(NULL)
	, m_callback(&callback)
	, m_send_buffer(NULL)
	, m_zero_registered(false)
	, m_pending_error(NULL)
{
	CheckQueueAvailable();
	InitBuffer();
	InitSemaphore();
}

RawChannel::RawChannel(Controller &con)
	: Mode(con, Controller::RawChannel)
	, m_mutex_valid(false)
	, m_cv_valid(false)
	, m_semaphore(NULL)
	, m_callback(NULL)
	, m_send_buffer(NULL)
	, m_zero_registered(false)
	, m_pending_error(NULL)
{
	CheckQueueAvailable();
	InitBuffer();
	InitSemaphore();
}

void RawChannel::CheckQueueAvailable()
{
	if( !m_con.HasQueue() ) {
		throw Barry::Error("RawChannel: No routing queue set in controller");
	}
}

void RawChannel::InitBuffer()
{
	m_send_buffer = new unsigned char[SB_CHANNELPACKET_HEADER_SIZE + SB_CHANNELPACKET_MAX_DATA_SIZE];
}

void RawChannel::InitSemaphore()
{
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

	delete[] m_send_buffer;

	if( m_mutex_valid ) {
		pthread_mutex_destroy(&m_mutex);
	}
	if( m_cv_valid ) {
		pthread_cond_destroy(&m_cv);
	}
	delete m_semaphore;
	delete m_pending_error;
}

void RawChannel::OnOpen()
{
	// Enable sequence packets so that DataSendAck callback and close can be
	// implemented
	m_zero_registered = true;
	SocketRoutingQueue::SocketDataHandlerPtr zeroCallback;
	zeroCallback.reset(new RawChannelZeroSocketHandler(*this));
	m_con.GetQueue()->RegisterInterest(0, zeroCallback);
	// Get socket data packets routed to this class as well if a
	// callback was provided, otherside just get the data packets
	// placed into a queue for the socket.
	if( m_callback ) {
		SocketRoutingQueue::SocketDataHandlerPtr callback;
		callback.reset(new RawChannelSocketHandler(*this));
		m_socket->UnregisterInterest();
		m_socket->RegisterInterest(callback);
	}
	else {
		// sockets already register themselves by default,
		// so no need to do anything in this case
	}
}


void RawChannel::HandleReceivedZeroPacket(Data &data)
{
	Protocol::CheckSize(data, SB_PACKET_HEADER_SIZE);
	MAKE_PACKETPTR_BUF(packet, data.GetData());

	if( packet->socket != 0 ) {
		UnregisterZeroSocketInterest();
		SetPendingError("RawChannel: Got packet not for socket-zero");
		m_semaphore->Signal();
	}

	switch( btohs(packet->command) )
	{
	case SB_COMMAND_CLOSE_SOCKET:
	case SB_COMMAND_REMOTE_CLOSE_SOCKET:
		// Stop listening to socket 0 messages
		// so that socket close work.
		UnregisterZeroSocketInterest();
		if( m_callback ) {
			m_callback->ChannelClose();
		}

		m_semaphore->Signal();
		break;
	default:
		UnregisterZeroSocketInterest();
		if( m_callback ) {
			m_callback->ChannelError("RawChannel: Got unexpected socket zero packet");
		}
		else {
			SetPendingError("RawChannel: Got unexpected socket zero packet");
		}
		m_semaphore->Signal();
		break;
	}

}

void RawChannel::HandleReceivedData(Data &data)
{
	// Only ever called in callback mode
	ValidateDataPacket(data);
	MAKE_CHANNELPACKETPTR_BUF(packet, data.GetData());

	// Check for sequence packets
	if( Protocol::IsSequencePacket(data) ) {
		m_semaphore->Signal();
		return;
	}

	// Should be a socket packet for us, so remove packet headers
	Data partial(packet->u.data, data.GetSize() - SB_CHANNELPACKET_HEADER_SIZE);
	if( m_callback ) {
		m_callback->DataReceived(partial);
	}
	else {
		SetPendingError("RawChannel: Received data to handle when in non-callback mode");
	}
}

void RawChannel::HandleError(Barry::Error &error)
{
	std::ostringstream errorOss;
	errorOss << "RawChannel: Socket error received, what: " << error.what();

	if( m_callback ) {
		m_callback->ChannelError(errorOss.str().c_str());
	}
	else {
		SetPendingError(errorOss.str().c_str());
	}
	m_semaphore->Signal();
}

void RawChannel::UnregisterZeroSocketInterest()
{
	if( m_zero_registered ) {
		m_con.GetQueue()->UnregisterInterest(0);
		m_zero_registered = false;
	}
}

void RawChannel::SetPendingError(const char *msg)
{
	if( !m_pending_error ) {
		m_pending_error = new std::string(msg);
	}
}

///////////////////////////////////////////////////////////////////////////////
// public API

void RawChannel::Send(Data &data, int timeout)
{
	size_t packetSize = SB_CHANNELPACKET_HEADER_SIZE + data.GetSize();

	if( packetSize > SB_CHANNELPACKET_HEADER_SIZE + SB_CHANNELPACKET_MAX_DATA_SIZE ) {
		throw Barry::Error("RawChannel: send data size larger than MaximumPacketSize");
	}

	// setup header and copy data in
	MAKE_CHANNELPACKETPTR_BUF(packet, m_send_buffer);
	packet->size = htobs(packetSize);
	std::memcpy(packet->u.data, data.GetData(), data.GetSize());

	Data toSend(m_send_buffer, packetSize);
	m_socket->SyncSend(toSend, timeout);
	if( m_callback ) {
		m_semaphore->WaitForSignal();
		if( m_pending_error ) {
			throw Barry::Error(*m_pending_error);
		}
	}
}

void RawChannel::Receive(Data &data,int timeout)
{
	if( m_callback ) {
		throw std::logic_error("RawChannel: Receive called when channel was created with a callback");
	}
	// Receive into a buffer
	m_socket->Receive(m_receive_data, timeout);
	// Then transfer across, skipping the header
	ValidateDataPacket(m_receive_data);
	MAKE_CHANNELPACKETPTR_BUF(packet, m_receive_data.GetData());

	size_t len = packet->size - SB_CHANNELPACKET_HEADER_SIZE;
	memcpy(data.GetBuffer(), packet->u.data, len);
	data.ReleaseBuffer(len);

}

void RawChannel::ValidateDataPacket(Data &data)
{
	Protocol::CheckSize(data, SB_CHANNELPACKET_HEADER_SIZE);
	MAKE_CHANNELPACKETPTR_BUF(packet, data.GetData());
	if( packet->size != data.GetSize() ) {

		throw std::logic_error("RawChannel: Data size doesn't match packet size");
	}
}

size_t RawChannel::MaximumSendSize()
{
	return SB_CHANNELPACKET_MAX_DATA_SIZE;
}

}} // namespace Barry::Mode
