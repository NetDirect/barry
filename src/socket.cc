///
/// \file	socket.cc
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

#include "i18n.h"
#include "socket.h"
#include "usbwrap.h"
#include "data.h"
#include "protocol.h"
#include "protostructs.h"
#include "endian.h"
#include "debug.h"
#include "packet.h"
#include "sha1.h"
#include <sstream>
#include <string.h>

using namespace Usb;


namespace Barry {


//////////////////////////////////////////////////////////////////////////////
// SocketZero class

SocketZero::SocketZero(	SocketRoutingQueue &queue,
			int writeEndpoint,
			uint8_t zeroSocketSequenceStart)
	: m_dev(0)
	, m_queue(&queue)
	, m_writeEp(writeEndpoint)
	, m_readEp(0)
	, m_zeroSocketSequence(zeroSocketSequenceStart)
	, m_sequenceId(0)
	, m_halfOpen(false)
	, m_challengeSeed(0)
	, m_remainingTries(0)
	, m_modeSequencePacketSeen(false)
	, m_pushback(false)
{
}

SocketZero::SocketZero(	Device &dev,
			int writeEndpoint, int readEndpoint,
			uint8_t zeroSocketSequenceStart)
	: m_dev(&dev)
	, m_queue(0)
	, m_writeEp(writeEndpoint)
	, m_readEp(readEndpoint)
	, m_zeroSocketSequence(zeroSocketSequenceStart)
	, m_sequenceId(0)
	, m_halfOpen(false)
	, m_challengeSeed(0)
	, m_remainingTries(0)
	, m_modeSequencePacketSeen(false)
	, m_pushback(false)
{
}

SocketZero::~SocketZero()
{
	// nothing to close for socket zero
}


///////////////////////////////////////
// Socket Zero static calls

// appends fragment to whole... if whole is empty, simply copies, and
// sets command to DATA instead of FRAGMENTED.  Always updates the
// packet size of whole, to reflect the total size
void SocketZero::AppendFragment(Data &whole, const Data &fragment)
{
	if( whole.GetSize() == 0 ) {
		// empty, so just copy
		whole = fragment;
	}
	else {
		// has some data already, so just append
		int size = whole.GetSize();
		unsigned char *buf = whole.GetBuffer(size + fragment.GetSize());
		MAKE_PACKET(fpack, fragment);
		int fragsize = fragment.GetSize() - SB_FRAG_HEADER_SIZE;

		memcpy(buf+size, &fpack->u.db.u.fragment, fragsize);
		whole.ReleaseBuffer(size + fragsize);
	}

	// update whole's size and command type for future sanity
	Barry::Protocol::Packet *wpack = (Barry::Protocol::Packet *) whole.GetBuffer();
	wpack->size = htobs((uint16_t) whole.GetSize());
	wpack->command = SB_COMMAND_DB_DATA;
	// don't need to call ReleaseBuffer here, since we're not changing
	// the real data size, and ReleaseBuffer was called above during copy
}

// If offset is 0, starts fresh, taking the first fragment packet size chunk
// out of whole and creating a sendable packet in fragment.  Returns the
// next offset if there is still more data, or 0 if finished.
unsigned int SocketZero::MakeNextFragment(const Data &whole, Data &fragment, unsigned int offset)
{
	// sanity check
	if( whole.GetSize() < SB_FRAG_HEADER_SIZE ) {
		eout("Whole packet too short to fragment: " << whole.GetSize());
		throw Error(_("Socket: Whole packet too short to fragment"));
	}

	// calculate size
	unsigned int todo = whole.GetSize() - SB_FRAG_HEADER_SIZE - offset;
	unsigned int nextOffset = 0;
	if( todo > (MAX_PACKET_SIZE - SB_FRAG_HEADER_SIZE) ) {
		todo = MAX_PACKET_SIZE - SB_FRAG_HEADER_SIZE;
		nextOffset = offset + todo;
	}

	// create fragment header
	unsigned char *buf = fragment.GetBuffer(SB_FRAG_HEADER_SIZE + todo);
	memcpy(buf, whole.GetData(), SB_FRAG_HEADER_SIZE);

	// copy over a fragment size of data
	memcpy(buf + SB_FRAG_HEADER_SIZE, whole.GetData() + SB_FRAG_HEADER_SIZE + offset, todo);

	// update fragment's size and command type
	Barry::Protocol::Packet *wpack = (Barry::Protocol::Packet *) buf;
	wpack->size = htobs((uint16_t) (todo + SB_FRAG_HEADER_SIZE));
	if( nextOffset )
		wpack->command = SB_COMMAND_DB_FRAGMENTED;
	else
		wpack->command = SB_COMMAND_DB_DATA;

	// adjust the new fragment size
	fragment.ReleaseBuffer(SB_FRAG_HEADER_SIZE + todo);

	// return next round
	return nextOffset;
}


///////////////////////////////////////
// SocketZero private API

//
// FIXME - not sure yet whether sequence ID's are per socket or not... if
// they are per socket, then this global sequence behaviour will not work,
// and we need to track m_sequenceId on a Socket level.
//
void SocketZero::CheckSequence(uint16_t socket, const Data &seq)
{
	MAKE_PACKET(spack, seq);
	if( (unsigned int) seq.GetSize() < SB_SEQUENCE_PACKET_SIZE ) {
		eout("Short sequence packet:\n" << seq);
		throw Error(_("Socket: invalid sequence packet"));
	}

	// we'll cheat here... if the packet's sequence is 0, we'll
	// silently restart, otherwise, fail
	uint32_t sequenceId = btohl(spack->u.sequence.sequenceId);
	if( sequenceId == 0 ) {
		// silently restart (will advance below)
		m_sequenceId = 0;
	}
	else {
		if( sequenceId != m_sequenceId ) {
			if( socket != 0 ) {
				std::string msg = string_vprintf(_("Socket 0x%x: out of sequence. (Global sequence: 0x%x. Packet sequence: 0x%x)"),
					(unsigned int)socket,
					(unsigned int)m_sequenceId,
					(unsigned int)sequenceId);
				eout(msg);
				throw Error(msg);
			}
			else {
				dout("Bad sequence on socket 0: expected: "
					<< m_sequenceId
					<< ". Packet sequence: " << sequenceId);
			}
		}
	}

	// advance!
	m_sequenceId++;
}

void SocketZero::SendOpen(uint16_t socket, Data &receive)
{
	// build open command
	Barry::Protocol::Packet packet;
	packet.socket = 0;
	packet.size = htobs(SB_SOCKET_PACKET_HEADER_SIZE);
	packet.command = SB_COMMAND_OPEN_SOCKET;
	packet.u.socket.socket = htobs(socket);
	packet.u.socket.sequence = m_zeroSocketSequence;// overwritten by Send()

	Data send(&packet, SB_SOCKET_PACKET_HEADER_SIZE);
	try {
		RawSend(send);
		RawReceive(receive);
		if( Protocol::IsSequencePacket(receive) ) {
			m_modeSequencePacketSeen = true;
			// during open, we could get a sequence packet in
			// the middle, from the SelectMode operation
			RawReceive(receive);
		}
	} catch( Usb::Error & ) {
		eeout(send, receive);
		throw;
	}

	// receive now holds the Open response
}

// SHA1 hashing logic based on Rick Scott's XmBlackBerry's send_password()
void SocketZero::SendPasswordHash(uint16_t socket, const char *password, Data &receive)
{
	unsigned char pwdigest[SHA_DIGEST_LENGTH];
	unsigned char prefixedhash[SHA_DIGEST_LENGTH + 4];

	// first, hash the password by itself
	SHA1((unsigned char *) password, strlen(password), pwdigest);

	// prefix the resulting hash with the provided seed
	uint32_t seed = htobl(m_challengeSeed);
	memcpy(&prefixedhash[0], &seed, sizeof(uint32_t));
	memcpy(&prefixedhash[4], pwdigest, SHA_DIGEST_LENGTH);

	// hash again
	SHA1((unsigned char *) prefixedhash, SHA_DIGEST_LENGTH + 4, pwdigest);


	size_t size = SB_SOCKET_PACKET_HEADER_SIZE + PASSWORD_CHALLENGE_SIZE;

	// build open command
	Barry::Protocol::Packet packet;
	packet.socket = 0;
	packet.size = htobs(size);
	packet.command = SB_COMMAND_PASSWORD;
	packet.u.socket.socket = htobs(socket);
	packet.u.socket.sequence = m_zeroSocketSequence;// overwritten by Send()
	packet.u.socket.u.password.remaining_tries = 0;
	packet.u.socket.u.password.unknown = 0;
	packet.u.socket.u.password.param = htobs(0x14);	// FIXME - what does this mean?
	memcpy(packet.u.socket.u.password.u.hash, pwdigest,
		sizeof(packet.u.socket.u.password.u.hash));

	// blank password hashes as we don't need these anymore
	memset(pwdigest, 0, sizeof(pwdigest));
	memset(prefixedhash, 0, sizeof(prefixedhash));

	Data send(&packet, size);
	RawSend(send);
	RawReceive(receive);

	// blank password hash as we don't need this anymore either
	memset(packet.u.socket.u.password.u.hash, 0,
		sizeof(packet.u.socket.u.password.u.hash));
	send.Zap();

	// check sequence ID
	if( Protocol::IsSequencePacket(receive) ) {
		m_modeSequencePacketSeen = true;

		CheckSequence(0, receive);

		// still need our ACK
		RawReceive(receive);
	}

	// receive now holds the Password response
}

void SocketZero::RawSend(Data &send, int timeout)
{
	Usb::Device *dev = m_queue ? m_queue->GetUsbDevice() : m_dev;
	if( !dev )
		throw Error(_("SocketZero: No device available for RawSend"));

	// Special case: it seems that sending packets with a size that's an
	// exact multiple of 0x40 causes the device to get confused.
	//
	// To get around that, it is observed in the captures that the size
	// is sent in a special 3 byte packet before the real packet.
	// Check for this case here.
	//
	if( (send.GetSize() % 0x40) == 0 ) {
		Protocol::SizePacket packet;
		packet.size = htobs(send.GetSize());
		packet.buffer[2] = 0;		// zero the top byte
		Data sizeCommand(&packet, 3);

		dev->BulkWrite(m_writeEp, sizeCommand, timeout);
	}

	dev->BulkWrite(m_writeEp, send, timeout);
}

void SocketZero::RawReceive(Data &receive, int timeout)
{
	if( m_pushback ) {
		receive = m_pushback_buffer;
		m_pushback = false;
		return;
	}

	if( m_queue ) {
		if( !m_queue->DefaultRead(receive, timeout) )
			throw Timeout(_("SocketZero::RawReceive: queue DefaultRead returned false (likely a timeout)"));
	}
	else {
		m_dev->BulkRead(m_readEp, receive, timeout);
	}

	ddout("SocketZero::RawReceive: Endpoint "
		<< (m_queue ? m_queue->GetReadEp() : m_readEp)
		<< "\nReceived:\n" << receive);
}

void SocketZero::Pushback(const Data &buf)
{
	if( m_pushback )
		throw Error(_("Multiple pushbacks in SocketZero!"));

	m_pushback = true;
	m_pushback_buffer = buf;
}


///////////////////////////////////////
// SocketZero public API

void SocketZero::SetRoutingQueue(SocketRoutingQueue &queue)
{
	// replace the current queue pointer
	m_queue = &queue;
}

void SocketZero::UnlinkRoutingQueue()
{
	m_queue = 0;
}

void SocketZero::Send(Data &send, int timeout)
{
	// force the socket number to 0
	if( send.GetSize() >= SB_SOCKET_PACKET_HEADER_SIZE ) {
		MAKE_PACKETPTR_BUF(spack, send.GetBuffer());
		spack->socket = 0;
	}

	// This is a socket 0 packet, so force the send packet data's
	// socket 0 sequence number to something correct.
	if( send.GetSize() >= SB_SOCKET_PACKET_HEADER_SIZE ) {
		MAKE_PACKETPTR_BUF(spack, send.GetBuffer());
		spack->u.socket.sequence = m_zeroSocketSequence;
		m_zeroSocketSequence++;
	}

	RawSend(send, timeout);
}

void SocketZero::Send(Data &send, Data &receive, int timeout)
{
	Send(send, timeout);
	RawReceive(receive, timeout);
}

void SocketZero::Send(Barry::Packet &packet, int timeout)
{
	Send(packet.m_send, *packet.m_receive, timeout);
}

void SocketZero::Receive(Data &receive, int timeout)
{
	RawReceive(receive, timeout);
}


//
// Open
//
/// Open a logical socket on the device.
///
/// Both the socket number and the flag are based on the response to the
/// SELECT_MODE command.  See Controller::SelectMode() for more info
/// on this.
///
/// The packet sequence is normal for most socket operations.
///
///	- Down: command packet with OPEN_SOCKET
///	- Up: optional sequence handshake packet
///	- Up: command response, which repeats the socket and flag data
///		as confirmation
///
/// \exception	Barry::Error
///		Thrown on protocol error.
///
/// \exception	Barry::BadPassword
///		Thrown on invalid password, or not enough retries left
///		on device.
///
SocketHandle SocketZero::Open(uint16_t socket, const char *password)
{
	return Open(SocketRoutingQueue::SocketDataHandlerPtr(),
		socket, password);
}
//
// Open
//
/// Open a logical socket on the device.
///
/// Both the socket number and the flag are based on the response to the
/// SELECT_MODE command.  See Controller::SelectMode() for more info
/// on this.
///
/// This version of open includes a socket routing queue handler to
/// avoid the race condition with incoming data being lost if
/// the handler is only registered after the socket is opened.
///
/// The packet sequence is normal for most socket operations.
///
///	- Down: command packet with OPEN_SOCKET
///	- Up: optional sequence handshake packet
///	- Up: command response, which repeats the socket and flag data
///		as confirmation
///
/// \exception	Barry::Error
///		Thrown on protocol error.
///
/// \exception	Barry::BadPassword
///		Thrown on invalid password, or not enough retries left
///		on device.
///
SocketHandle SocketZero::Open(
	Barry::SocketRoutingQueue::SocketDataHandlerPtr handler,
	uint16_t socket, const char *password)
{
	// Things get a little funky here, as we may be left in an
	// intermediate state in the case of a failed password.
	// This function should support being called as many times
	// as needed to handle the password


	Data send, receive;
	ZeroPacket packet(send, receive);

	// save sequence for later close
	uint8_t closeFlag = GetZeroSocketSequence();

	// It's necessary to create the socket before performing any
	// device IO as the device can start sending packets for a socket
	// as soon as it's sent a SB_COMMAND_OPENED_SOCKET command.
	//
	// If something goes wrong and this method throws an error
	// then the SocketHandle cleanup will destroy the socket,
	// correctly unregistering it.
	Socket *sock = new Socket(*this, socket, closeFlag);
	SocketHandle sh(sock);

	sock->Opening(handler);

	if( !m_halfOpen ) {
		// starting fresh
		m_remainingTries = 0;

		// this gets set to true if we see a starting sequence packet
		// during any of our open and password commands... After
		// a mode command (like "RIM Desktop", etc.) a starting
		// sequence packet is sent, and may arrive before or after
		// the socket open handshake.
		m_modeSequencePacketSeen = false;

		// go for it!
		SendOpen(socket, receive);

		// check for password challenge, or success
		if( packet.Command() == SB_COMMAND_PASSWORD_CHALLENGE ) {
			m_halfOpen = true;
			m_challengeSeed = packet.ChallengeSeed();
			m_remainingTries = packet.RemainingTries();
		}

		// fall through to challenge code...
	}

	if( m_halfOpen ) {
		// half open, device is expecting a password hash... do we
		// have a password?
		if( !password ) {
			throw BadPassword(_("No password specified."), m_remainingTries, false);
		}

		// only allow password attempts if there are
		// BARRY_MIN_PASSWORD_TRIES or more tries remaining...
		// we want to give the user at least some chance on a
		// Windows machine before the device commits suicide.
		if( m_remainingTries < BARRY_MIN_PASSWORD_TRIES ) {
			throw BadPassword(string_vprintf(_("Fewer than %d password tries remaining in device. Refusing to proceed, to avoid device zapping itself.  Use a Windows client, or re-cradle the device."), BARRY_MIN_PASSWORD_TRIES),
				m_remainingTries,
				true);
		}

		// save sequence for later close (again after SendOpen())
		closeFlag = GetZeroSocketSequence();

		SendPasswordHash(socket, password, receive);

		if( packet.Command() == SB_COMMAND_PASSWORD_FAILED ) {
			m_halfOpen = true;
			m_challengeSeed = packet.ChallengeSeed();
			m_remainingTries = packet.RemainingTries();
			throw BadPassword(_("Password rejected by device."), m_remainingTries, false);
		}

		// if we get this far, we are no longer in half-open password
		// mode, so we can reset our flags
		m_halfOpen = false;

		// fall through to success check...
	}

	// If the device thinks that the socket was already open then
	// it will tell us by sending an SB_COMMAND_CLOSE_SOCKET.
	//
	// This happens most commonly when using raw channels which
	// haven't been cleanly closed (such as by killing the process
	// running Barry) and so the device still thinks the socket
	// is open.
	//
	// Retrying the open will usually succeed, but relies on the
	// device software re-creating the channel after it's closed
	// so return an error here instead of automatically retrying.
	if( packet.Command() == SB_COMMAND_CLOSE_SOCKET )
	{
		throw SocketCloseOnOpen(_("Socket: Device closed socket when trying to open (can be caused by the wrong password, or if the device thinks the socket is already open... please try again)"));
	}

	if( packet.Command() != SB_COMMAND_OPENED_SOCKET ||
	    packet.SocketResponse() != socket ||
	    packet.SocketSequence() != closeFlag )
	{
		eout("Packet:\n" << receive);
		throw Error(_("Socket: Bad OPENED packet in Open"));
	}

	// if no sequence packet has yet arrived, wait for it here
	if( !m_modeSequencePacketSeen ) {
		Data sequence;
		RawReceive(sequence);
		if( !Protocol::IsSequencePacket(sequence) ) {
			// if this is not the sequence packet, then it might
			// just be out of order (some devices do this when
			// opening the JavaLoader mode), so as a last
			// ditch effort, do one more read with a short
			// timeout, and check that as well
			Data late_sequence;
			RawReceive(late_sequence, 500);
			if( !Protocol::IsSequencePacket(late_sequence) ) {
				throw Error(_("Could not find mode's starting sequence packet"));
			}

			// ok, so our ditch effort worked, but now we have
			// a leftover packet on our hands... do a temporary
			// pushback
			Pushback(sequence);
		}
	}

	// success! 
	sock->Opened();

	return sh;
}

//
// Close
//
/// Closes a non-default socket (i.e. non-zero socket number)
///
/// The packet sequence is just like Open(), except the command is
/// CLOSE_SOCKET.
///
/// \exception	Barry::Error
///
void SocketZero::Close(Socket &socket)
{
	if( socket.GetSocket() == 0 )
		return;		// nothing to do

	// build close command
	Barry::Protocol::Packet packet;
	packet.socket = 0;
	packet.size = htobs(SB_SOCKET_PACKET_HEADER_SIZE);
	packet.command = SB_COMMAND_CLOSE_SOCKET;
	packet.u.socket.socket = htobs(socket.GetSocket());
	packet.u.socket.sequence = socket.GetCloseFlag();

	Data command(&packet, SB_SOCKET_PACKET_HEADER_SIZE);
	Data response;
	try {
		Send(command, response);
	}
	catch( Usb::Error & ) {
		// reset so this won't be called again
		socket.ForceClosed();

		eeout(command, response);
		throw;
	}

	// starting fresh, reset sequence ID
	if( Protocol::IsSequencePacket(response) ) {
		CheckSequence(0, response);

		// still need our ACK
		RawReceive(response);
	}

	Protocol::CheckSize(response, SB_SOCKET_PACKET_HEADER_SIZE);
	MAKE_PACKET(rpack, response);
	// The reply will be SB_COMMAND_CLOSED_SOCKET if the device
	// has closed the socket in response to our request.
	//
	// It's also possible for the reply to be
	// SB_COMMAND_REMOTE_CLOSE_SOCKET if the device wanted to
	// close the socket at the same time, such as if the channel
	// API is being used by the device.
	if( ( rpack->command != SB_COMMAND_CLOSED_SOCKET &&
	      rpack->command != SB_COMMAND_REMOTE_CLOSE_SOCKET ) ||
	    btohs(rpack->u.socket.socket) != socket.GetSocket() ||
	    rpack->u.socket.sequence != socket.GetCloseFlag() )
	{
		// reset so this won't be called again
		socket.ForceClosed();

		eout("Packet:\n" << response);
		throw BadPacket(rpack->command, _("Socket: Bad CLOSED packet in Close"));
	}

	if( socket.IsResetOnClose() ) {
		Data send, receive;
		ZeroPacket reset_packet(send, receive);
		reset_packet.Reset();

		Send(reset_packet);
		if( reset_packet.CommandResponse() != SB_COMMAND_RESET_REPLY ) {
			throw BadPacket(reset_packet.CommandResponse(),
				_("Socket: Missing RESET_REPLY in Close"));
		}
	}

//	// and finally, there always seems to be an extra read of
//	// an empty packet at the end... just throw it away
//	try {
//		RawReceive(response, 1);
//	}
//	catch( Usb::Timeout & ) {
//	}

	// reset socket and flag
	socket.ForceClosed();
}





//////////////////////////////////////////////////////////////////////////////
// SocketBase class

SocketBase::~SocketBase()
{
}

void SocketBase::CheckSequence(const Data &seq)
{
	// FIXME - needs implementation
}

//
// DBFragSend
//
/// Sends a fragmented Desktop / Database command packet.
/// Assumes that 'send' contains a valid packet, which may or may not
/// need fragmentation.  If it does, fragmentation will be done
/// automatically.
///
void SocketBase::DBFragSend(Data &send, int timeout)
{
	MAKE_PACKET(spack, send);
	if( send.GetSize() < MIN_PACKET_SIZE ||
	    (spack->command != SB_COMMAND_DB_DATA &&
	     spack->command != SB_COMMAND_DB_DONE) )
	{
		// we don't do that around here
		eout("unknown send data in DBFragSend(): " << send);
		throw std::logic_error(_("Socket: unknown send data in DBFragSend()"));
	}

	if( send.GetSize() <= MAX_PACKET_SIZE ) {
		// send non-fragmented
		SyncSend(send, timeout);
	}
	else {
		// send fragmented
		unsigned int offset = 0;
		Data outFrag;

		do {
			offset = SocketZero::MakeNextFragment(send, outFrag, offset);
			SyncSend(outFrag, timeout);
		} while( offset > 0 );
	}

}

//
// Send
//
/// SyncSends 'send' data to device, and waits for response.
///
/// \returns	void
///
/// \exception	Usb::Error on underlying bus errors.
///
void SocketBase::Send(Data &send, Data &receive, int timeout)
{
	SyncSend(send, timeout);
	Receive(receive, timeout);
}

void SocketBase::Send(Barry::Packet &packet, int timeout)
{
	Send(packet.m_send, *packet.m_receive, timeout);
}

// sends the send packet down to the device, fragmenting if
// necessary, and returns the response in receive, defragmenting
// if needed
// Blocks until response received or timed out in Usb::Device
//
// This is primarily for Desktop Database packets... Javaloader
// packets use PacketData().
//
void SocketBase::Packet(Data &send, Data &receive, int timeout)
{
	// assume the common case of no fragmentation,
	// and use the receive buffer for input... allocate a frag buffer
	// later if necessary
	Data *inputBuf = &receive;
	receive.Zap();

	DBFragSend(send, timeout);
	Receive(*inputBuf, timeout);

	std::auto_ptr<Data> inFrag;
	bool done = false, frag = false;
	int blankCount = 0;
	while( !done ) {
		MAKE_PACKET(rpack, *inputBuf);

		// check the packet's validity
		if( inputBuf->GetSize() > 0 ) {
			blankCount = 0;

			Protocol::CheckSize(*inputBuf, SB_PACKET_HEADER_SIZE);

			switch( rpack->command )
			{
			case SB_COMMAND_SEQUENCE_HANDSHAKE:
				CheckSequence(*inputBuf);
				break;

			case SB_COMMAND_DB_DATA:
				if( frag ) {
					SocketZero::AppendFragment(receive, *inputBuf);
				}
				else {
					// no copy needed, already in receive,
					// since inputBuf starts out that way
				}
				done = true;
				break;

			case SB_COMMAND_DB_FRAGMENTED:
				// only copy if frag is true, since the
				// first time through, receive == inputBuf
				if( frag ) {
					SocketZero::AppendFragment(receive, *inputBuf);
				}
				frag = true;
				break;

			case SB_COMMAND_DB_DONE:
				// no copy needed, already in receive
				done = true;
				break;

			default: {
				std::ostringstream oss;
				oss << _("Socket: (read) unhandled packet in Packet(): ") << "0x" << std::hex << (unsigned int)rpack->command;
				eout(oss.str());
				throw Error(oss.str());
				}
				break;
			}
		}
		else {
			blankCount++;
			//std::cerr << "Blank! " << blankCount << std::endl;
			if( blankCount == 10 ) {
				// only ask for more data on stalled sockets
				// for so long
				throw Error(_("Socket: 10 blank packets received"));
			}
		}

		if( !done ) {
			// not done yet, ask for another read, and
			// create new buffer for fragmented reads
			if( frag && !inFrag.get() ) {
				inFrag.reset( new Data );
				inputBuf = inFrag.get();
			}
			Receive(*inputBuf);
		}
	}
}

void SocketBase::Packet(Barry::Packet &packet, int timeout)
{
	Packet(packet.m_send, *packet.m_receive, timeout);
}

void SocketBase::Packet(Barry::JLPacket &packet, int timeout)
{
	if( packet.HasData() ) {
		SyncSend(packet.m_cmd);
		PacketData(packet.m_data, *packet.m_receive, false, timeout);
	}
	else {
		PacketData(packet.m_cmd, *packet.m_receive, false, timeout);
	}
}

void SocketBase::Packet(Barry::JVMPacket &packet, int timeout)
{
	PacketJVM(packet.m_cmd, *packet.m_receive, timeout);
}

// sends the send packet down to the device
// Blocks until response received or timed out in Usb::Device
//
// This function is used to send packet to JVM
void SocketBase::PacketJVM(Data &send, Data &receive, int timeout)
{
	if( ( send.GetSize() < MIN_PACKET_DATA_SIZE ) ||
		( send.GetSize() > MAX_PACKET_DATA_SIZE ) ) {
		// we don't do that around here
		throw std::logic_error(_("Socket: unknown send data in PacketJVM()"));
	}

	Data &inFrag = receive;
	receive.Zap();

	// send non-fragmented
	RawSend(send, timeout);
	Receive(inFrag, timeout);

	bool done = false;
	int blankCount = 0;

	while( !done ) {
		// check the packet's validity
		if( inFrag.GetSize() > 6 ) {
			MAKE_PACKET(rpack, inFrag);

			blankCount = 0;

			Protocol::CheckSize(inFrag, SB_PACKET_HEADER_SIZE);

			switch( rpack->command )
			{
			case SB_COMMAND_SEQUENCE_HANDSHAKE:
				CheckSequence(inFrag);
				break;

			default: {
				std::ostringstream oss;
				oss << _("Socket: (read) unhandled packet in Packet(): ") << "0x" << std::hex << (unsigned int)rpack->command;
				eout(oss.str());
				throw Error(oss.str());
				}
				break;
			}
		}
		else if( inFrag.GetSize() == 6 ) {
			done = true;
		}
		else {
			blankCount++;

			//std::cerr << "Blank! " << blankCount << std::endl;
			if( blankCount == 10 ) {
				// only ask for more data on stalled sockets
				// for so long
				throw Error(_("Socket: 10 blank packets received"));
			}
		}

		if( !done ) {
			// not done yet, ask for another read
			Receive(inFrag, timeout);
		}
	}
}

// sends the send packet down to the device
// Blocks until response received or timed out in Usb::Device
void SocketBase::PacketData(Data &send,
				Data &receive,
				bool done_on_sequence,
				int timeout)
{
	if( ( send.GetSize() < MIN_PACKET_DATA_SIZE ) ||
		( send.GetSize() > MAX_PACKET_DATA_SIZE ) ) {
		// we don't do that around here
		throw std::logic_error(_("Socket: unknown send data in PacketData()"));
	}

	Data &inFrag = receive;
	receive.Zap();

	// send non-fragmented
	SyncSend(send, timeout);
	Receive(inFrag, timeout);

	bool done = false;
	int blankCount = 0;

	while( !done ) {
		// check the packet's validity
		if( inFrag.GetSize() > 0 ) {
			MAKE_PACKET(rpack, inFrag);

			blankCount = 0;

			Protocol::CheckSize(inFrag, SB_PACKET_HEADER_SIZE);

			switch( rpack->command )
			{
			case SB_COMMAND_SEQUENCE_HANDSHAKE:
//				CheckSequence(inFrag);
				if( done_on_sequence )
					done = true;
				break;

			case SB_COMMAND_JL_READY:
			case SB_COMMAND_JL_ACK:
			case SB_COMMAND_JL_HELLO_ACK:
			case SB_COMMAND_JL_RESET_REQUIRED:
				done = true;
				break;

			case SB_COMMAND_JL_GET_DATA_ENTRY:	// This response means that the next packet is the stream
				done = true;
				break;

			case SB_DATA_JL_INVALID:
				throw BadPacket(rpack->command, _("file is not a valid Java code file"));
				break;

			case SB_COMMAND_JL_NOT_SUPPORTED:
				throw BadPacket(rpack->command, _("device does not support requested command"));
				break;

			default:
				// unknown packet, pass it up to the
				// next higher code layer
				done = true;
				break;
			}
		}
		else {
			blankCount++;
			//std::cerr << "Blank! " << blankCount << std::endl;
			if( blankCount == 10 ) {
				// only ask for more data on stalled sockets
				// for so long
				throw Error(_("Socket: 10 blank packets received"));
			}
		}

		if( !done ) {
			// not done yet, ask for another read
			Receive(inFrag);
		}
	}
}

void SocketBase::NextRecord(Data &receive)
{
	Barry::Protocol::Packet packet;
	packet.size = htobs(7);
	packet.command = SB_COMMAND_DB_DONE;
	packet.u.db.tableCmd = 0;
	packet.u.db.u.command.operation = 0;

	Data command(&packet, 7);
	Packet(command, receive);
}



//////////////////////////////////////////////////////////////////////////////
// Socket class

Socket::Socket( SocketZero &zero,
		uint16_t socket,
		uint8_t closeFlag)
	: m_zero(&zero)
	, m_socket(socket)
	, m_closeFlag(closeFlag)
	, m_registered(false)
	, m_sequence(new Data)
{
}

Socket::~Socket()
{
	// trap exceptions in the destructor
	try {
		// a non-default socket has been opened, close it
		LocalClose();
	}
	catch( std::runtime_error &DEBUG_ONLY(re) ) {
		// do nothing... log it?
		dout("Exception caught in ~Socket: " << re.what());
	}
}


////////////////////////////////////
// Socket protected API

void Socket::ForceClosed()
{
	m_socket = 0;
	m_closeFlag = 0;
}

void Socket::LocalClose()
{
	LocalUnregisterInterest();
	m_zero->Close(*this);
}

void Socket::LocalUnregisterInterest()
{
	if( m_registered ) {
		if( m_zero->m_queue )
			m_zero->m_queue->UnregisterInterest(m_socket);
		m_registered = false;
	}
}


//
// Send
//
/// Sends 'send' data to device, no receive.
///
/// \returns	void
///
/// \exception	Usb::Error on underlying bus errors.
///
void Socket::RawSend(Data &send, int timeout)
{
	// force the socket number to this socket
	if( send.GetSize() >= SB_PACKET_HEADER_SIZE ) {
		MAKE_PACKETPTR_BUF(spack, send.GetBuffer());
		spack->socket = htobs(m_socket);
	}
	m_zero->RawSend(send, timeout);
}

void Socket::SyncSend(Data &send, int timeout)
{
	RawSend(send, timeout);
	Receive(*m_sequence, timeout);
	if( !Protocol::IsSequencePacket(*m_sequence) )
		throw Barry::Error(_("Non-sequence packet in Socket::SyncSend()"));
	CheckSequence(*m_sequence);
}

void Socket::Receive(Data &receive, int timeout)
{
	if( m_registered ) {
		if( m_zero->m_queue ) {
			if( !m_zero->m_queue->SocketRead(m_socket, receive, timeout) )
				throw Timeout(_("Socket::Receive: queue SocketRead returned false (likely a timeout)"));
		}
		else {
			throw std::logic_error(_("NULL queue pointer in a registered socket read."));
		}
		ddout("Socket::Receive: Endpoint "
			<< (m_zero->m_queue ? m_zero->m_queue->GetReadEp() : m_zero->m_readEp)
			<< "\nReceived:\n" << receive);
	}
	else {
		m_zero->RawReceive(receive, timeout);
	}
}


void Socket::RegisterInterest(SocketRoutingQueue::SocketDataHandlerPtr handler)
{
	if( !m_zero->m_queue )
		throw std::logic_error(_("SocketRoutingQueue required in SocketZero in order to call Socket::RegisterInterest()"));

	if( m_registered ) {
		throw std::logic_error(string_vprintf(_("Socket (%u) already registered in Socket::RegisterInterest()!"), (unsigned int)m_socket));
	}

	m_zero->m_queue->RegisterInterest(m_socket, handler);
	m_registered = true;
}

void Socket::Opening(Barry::SocketRoutingQueue::SocketDataHandlerPtr handler)
{
	// if we are running with a routing queue, register the
	// socket's interest in its own data packets.  By default, this
	// data will be queued without a callback handler.
	// If other application code needs to intercept this with
	// its own handler, it must call UnregisterInterest() and
	// re-register its own handler
	//
	// As opening requires sequence packets to still come in on
	// the default queue, only register for data until the socket
	// is opened.
	if( m_zero->m_queue ) {
		m_zero->m_queue->RegisterInterestAndType(m_socket,
			handler, SocketRoutingQueue::DataPackets);
		m_registered = true;
	}
}

void Socket::Opened()
{
	if( m_zero->m_queue ) {
		if( !m_registered ) {
			std::ostringstream oss;
			oss << "Socket (" << m_socket << ") not previously registered in Socket::Opened()!";
			throw std::logic_error(oss.str());
		}
		// Need to complete registration and register for 
		// sequence packets in addition to data packets.
		m_zero->m_queue->ChangeInterest(m_socket,
			SocketRoutingQueue::SequenceAndDataPackets);
	}
}


} // namespace Barry

