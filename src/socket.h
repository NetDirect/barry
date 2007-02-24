///
/// \file	socket.h
///		Class wrapper to encapsulate the Blackberry USB logical socket
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <stdint.h>

// forward declarations
namespace Usb { class Device; }
namespace Barry { class Data; class Packet; }

namespace Barry {

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
class Socket
{
	Usb::Device &m_dev;
	int m_writeEp, m_readEp;
	uint16_t m_socket;		// defaults to 0, which is valid,
					// since socket 0 is always open
					// If this is not 0, then class will
					// deal with closing automatically.
	uint8_t m_zeroSocketSequence;
	uint8_t m_flag;
	uint32_t m_sequenceId;

	// half open socket stata, for passwords
	bool m_halfOpen;
	uint32_t m_challengeSeed;
	unsigned int m_remainingTries;

private:
	// sends 'send' data to device, and waits for response, using
	// "read first, write second" order observed in capture
	void AppendFragment(Data &whole, const Data &fragment);
	unsigned int MakeNextFragment(const Data &whole, Data &fragment, unsigned int offset = 0);
	void CheckSequence(const Data &seq);

	void SendOpen(uint16_t socket, Data &receive);
	void SendPasswordHash(uint16_t socket, const char *password, Data &receive);

public:
	Socket(Usb::Device &dev, int writeEndpoint, int readEndpoint,
		uint8_t zeroSocketSequenceStart = 0);
	~Socket();

	uint16_t GetSocket() const { return m_socket; }
	uint8_t GetZeroSocketSequence() const { return m_zeroSocketSequence; }

	void Open(uint16_t socket, const char *password = 0);
	void Close();

	// Send and Receive are available before Open...
	// an unopened socket defaults to socket 0, which you need
	// in order to set the blackberry mode
	// The send function will overwrite the zeroSocketSequence byte
	// *inside* the packet, if the current m_socket is 0.
	void Send(Data &send, Data &receive, int timeout = -1);
	void Send(Barry::Packet &packet, int timeout = -1);
	void Receive(Data &receive, int timeout = -1);

	// sends the send packet down to the device, fragmenting if
	// necessary, and returns the response in receive, defragmenting
	// if needed
	// Blocks until response received or timed out in Usb::Device
	void Packet(Data &send, Data &receive, int timeout = -1);
	void Packet(Barry::Packet &packet, int timeout = -1);

	// some handy wrappers for the Packet() interface
	void NextRecord(Data &receive);
};


} // namespace Barry

#endif

