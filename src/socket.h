///
/// \file	socket.h
///		Class wrapper to encapsulate the Blackberry USB logical socket
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SOCKET_H__
#define __BARRY_SOCKET_H__

#include "dll.h"
#include <stdint.h>
#include <queue>
#include <memory>
#include "router.h"
#include "data.h"

// forward declarations
namespace Usb { class Device; }
namespace Barry {
	class Packet;
	class JLPacket;
	class JVMPacket;
	class SocketRoutingQueue;
}

namespace Barry {

class SocketBase;
class Socket;
typedef std::auto_ptr<SocketBase>	SocketHandle;

class BXEXPORT SocketZero
{
	friend class SocketBase;
	friend class Socket;

	Usb::Device *m_dev;
	SocketRoutingQueue *m_queue;
	int m_writeEp, m_readEp;
	uint8_t m_zeroSocketSequence;

	uint32_t m_sequenceId;

	// half open socket stata, for passwords
	bool m_halfOpen;
	uint32_t m_challengeSeed;
	unsigned int m_remainingTries;
	bool m_modeSequencePacketSeen;

	// pushback for out of order sequence packets
	Data m_pushback_buffer;
	bool m_pushback;

private:
	static void AppendFragment(Data &whole, const Data &fragment);
	static unsigned int MakeNextFragment(const Data &whole, Data &fragment,
		unsigned int offset = 0);
	void CheckSequence(uint16_t socket, const Data &seq);

	void SendOpen(uint16_t socket, Data &receive);
	void SendPasswordHash(uint16_t socket, const char *password, Data &receive);

	// Raw send and receive functions, used for all low level
	// communication to the USB level.
	void RawSend(Data &send, int timeout = -1);
	void RawReceive(Data &receive, int timeout = -1);

	void Pushback(const Data &buf);

protected:
	bool IsSequencePacketHidden() { return false; }

public:
	explicit SocketZero(SocketRoutingQueue &queue, int writeEndpoint,
		uint8_t zeroSocketSequenceStart = 0);
	SocketZero(Usb::Device &dev, int writeEndpoint, int readEndpoint,
		uint8_t zeroSocketSequenceStart = 0);
	~SocketZero();

	uint8_t GetZeroSocketSequence() const { return m_zeroSocketSequence; }

	void SetRoutingQueue(SocketRoutingQueue &queue);
	void UnlinkRoutingQueue();

	// Send functions for socket 0 only.
	// These functions will overwrite:
	//     - the zeroSocketSequence byte *inside* the packet
	//     - the socket number to 0
	//
	void Send(Data &send, int timeout = -1);	// send only
	void Send(Data &send, Data &receive, int timeout = -1); // send+recv
	void Send(Barry::Packet &packet, int timeout = -1);
	void Receive(Data &receive, int timeout = -1);

	// Opens a new socket and returns a Socket object to manage it
	SocketHandle Open(uint16_t socket, const char *password = 0);
	// Opens a new socket and returns a Socket object to manage it
	// Registers the provided handler interested in socket data.
	// This avoids the race where calling RegisterInterest immediately
	// on the returned SocketHandle can still miss incoming data.
	SocketHandle Open(
		Barry::SocketRoutingQueue::SocketDataHandlerPtr handler,
		uint16_t socket, const char *password = 0);
	void Close(Socket &socket);
};

class BXEXPORT SocketBase
{
	bool m_resetOnClose;

protected:
	void CheckSequence(const Data &seq);

public:
	SocketBase()
		: m_resetOnClose(false)
	{
	}

	virtual ~SocketBase();

	//
	// Virtual Socket API
	//
	virtual void Close() = 0;

	// FIXME - do I need RawSend?  Or just a good fragmenter?
	virtual void RawSend(Data &send, int timeout = -1) = 0;
	virtual void SyncSend(Data &send, int timeout = -1) = 0;
	virtual void Receive(Data &receive, int timeout = -1) = 0;

	virtual void RegisterInterest(Barry::SocketRoutingQueue::SocketDataHandlerPtr handler = Barry::SocketRoutingQueue::SocketDataHandlerPtr()) = 0;
	virtual void UnregisterInterest() = 0;

	// Used to notify the socket when the connection is being opened
	virtual void Opening(Barry::SocketRoutingQueue::SocketDataHandlerPtr handler) = 0;
	// Used to notify the socket when the connection has been successfully opened
	virtual void Opened() = 0;

	void ResetOnClose(bool reset = true) { m_resetOnClose = reset; }
	bool IsResetOnClose() const { return m_resetOnClose; }

	//
	// Convenience functions that just call the virtuals above
	//
	void DBFragSend(Data &send, int timeout = -1);
	void Send(Data &send, Data &receive, int timeout = -1);
	void Send(Barry::Packet &packet, int timeout = -1);

	//
	// Protocol based functions... all use the above virtual functions
	//

	// sends the send packet down to the device, fragmenting if
	// necessary, and returns the response in receive, defragmenting
	// if needed
	// Blocks until response received or timed out in Usb::Device
	void Packet(Data &send, Data &receive, int timeout = -1);
	void Packet(Barry::Packet &packet, int timeout = -1);
	void Packet(Barry::JLPacket &packet, int timeout = -1);
	void Packet(Barry::JVMPacket &packet, int timeout = -1);

	// Use this function to send packet to JVM instead of Packet function
	// FIXME
	void PacketJVM(Data &send, Data &receive, int timeout = -1);

	// Use this function to send data packet instead of Packet function
	// Indeed, Packet function is used to send command (and not data)
	// FIXME
	void PacketData(Data &send, Data &receive, bool done_on_sequence,
		int timeout = -1);

	// some handy wrappers for the Packet() interface
	void NextRecord(Data &receive);
};

//
// Socket class
//
/// Encapsulates a "logical socket" in the Blackberry USB protocol.
/// By default, provides raw send/receive access, as well as packet
/// writing on socket 0, which is always open.
///
/// There are Open and Close members to open data sockets which are used
/// to transfer data to and from the device.
///
/// The destructor will close any non-0 open sockets automatically.
///
/// Requires an active Usb::Device object to work on.
///
class BXEXPORT Socket : public SocketBase
{
	friend class SocketZero;

	SocketZero *m_zero;
	uint16_t m_socket;
	uint8_t m_closeFlag;

	bool m_registered;

	// buffer data
	std::auto_ptr<Data> m_sequence;

protected:
	void ForceClosed();

	// These "Local" function are non-virtual worker functions,
	// which are safe to be called from the destructor.
	// If you override the virtual versions in your derived class,
	// make sure your "Local" versions call us.
	void LocalClose();
	void LocalUnregisterInterest();

	uint16_t GetSocket() const { return m_socket; }
	uint8_t GetCloseFlag() const { return m_closeFlag; }

	Socket(SocketZero &zero, uint16_t socket, uint8_t closeFlag);

public:
	~Socket();

	//
	// virtual overrides
	//
	void Close() { LocalClose(); }
	void RawSend(Data &send, int timeout = -1);
	void SyncSend(Data &send, int timeout = -1);
	void Receive(Data &receive, int timeout = -1);


	// Register a callback for incoming data from the device.
	// This function assumes that this socket is based on a socketZero
	// that has a SocketRoutingQueue, otherwise throws logic_error.
	// It also assumes that nothing has been registered before.
	// If you wish to re-register, call UnregisterInterest() first,
	// which is safe to call as many times as you like.
	void RegisterInterest(Barry::SocketRoutingQueue::SocketDataHandlerPtr handler = Barry::SocketRoutingQueue::SocketDataHandlerPtr());
	void UnregisterInterest() { LocalUnregisterInterest(); }

	void Opening(Barry::SocketRoutingQueue::SocketDataHandlerPtr handler);
	void Opened();
};


} // namespace Barry

#endif

