///
/// \file	socket.cc
///		Class wrapper to encapsulate the Blackberry USB logical socket
///


#include "usbwrap.h"
#include "protocol.h"
#include "socket.h"
#include "debug.h"
#include "data.h"
#include "error.h"


using namespace Usb;


namespace Syncberry {

Socket::Socket(Device &dev, int writeEndpoint, int readEndpoint)
	: m_dev(dev),
	m_writeEp(writeEndpoint),
	m_readEp(readEndpoint),
	m_socket(0),
	m_flag(0),
	m_sequenceId(0),
	m_lastStatus(0)
{
}

Socket::~Socket()
{
	// trap exceptions in the destructor
	try {
		// a non-default socket has been opened, close it
		Close();
	}
	catch( std::runtime_error &re ) {
		// do nothing... log it?
		dout("Exception caught in ~Socket: " << re.what());
	}
}

void Socket::Open(uint16_t socket, uint8_t flag)
{
	if( m_socket != 0 ) {
		// already open
		throw SBError("Socket: already open");
	}

	// build open command
	Syncberry::Packet packet;
	packet.socket = 0;
	packet.size = SB_SOCKET_PACKET_SIZE;
	packet.command = SB_COMMAND_OPEN_SOCKET;
	packet.data.simple.data.socket.socket = socket;
	packet.data.simple.data.socket.param = flag;

	Data send(&packet, packet.size);
	Data receive;
	if( !Send(send, receive) ) {
		eout("Send packet:\n" << send);
		eout("Receive packet: \n" << receive);
		throw SBError(GetLastStatus(), "Error opening socket");
	}

	// starting fresh, reset sequence ID
	CheckSize(receive);
	if( IS_COMMAND(receive, SB_COMMAND_SEQUENCE_HANDSHAKE) ) {
		CheckSequence(receive);

		// still need our ACK
		IO rd = m_dev.ABulkRead(m_readEp, receive);
		rd.Wait();
	}

	CheckSize(receive, SB_SOCKET_PACKET_SIZE);
	MAKE_PACKET(rpack, receive);
	if( rpack->command != SB_COMMAND_OPENED_SOCKET ||
	    rpack->data.simple.data.socket.socket != socket ||
	    rpack->data.simple.data.socket.param != flag )
	{
		eout("Packet:\n" << receive);
		throw SBError("Socket: Bad OPENED packet in Open");
	}
}

void Socket::Close()
{
	if( m_socket != 0 ) {
		// only close non-default sockets
	}
}

// sends 'send' data to device, and waits for response, using
// "read first, write second" order observed in capture
//
// returns true on success, on failure, use GetLastStatus() for kernel
// URB error code
bool Socket::Send(const Data &send, Data &receive)
{
	IO rd = m_dev.ABulkRead(m_readEp, receive);
	IO wr = m_dev.ABulkWrite(m_writeEp, send);

	// wait for response
	rd.Wait();
	wr.Wait();

	m_lastStatus = rd.GetStatus();
	receive.ReleaseBuffer(m_lastStatus >= 0 ? rd.GetSize() : 0);
	return m_lastStatus >= 0;
}

bool Socket::Receive(Data &receive)
{
	IO rd = m_dev.ABulkRead(m_readEp, receive);

	rd.Wait();

	m_lastStatus = rd.GetStatus();
	receive.ReleaseBuffer(m_lastStatus >= 0 ? rd.GetSize() : 0);
	return m_lastStatus >= 0;
}

// appends fragment to whole... if whole is empty, simply copies, and
// sets command to DATA instead of FRAGMENTED.  Always updates the
// packet size of whole, to reflect the total size
void Socket::AppendFragment(Data &whole, const Data &fragment)
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

		memcpy(buf+size, &fpack->data.param.data.raw, fragsize);
		whole.ReleaseBuffer(size + fragsize);
	}

	// update whole's size and command type for future sanity
	Syncberry::Packet *wpack = (Syncberry::Packet *) whole.GetBuffer();
	wpack->size = (uint16_t) whole.GetSize();
	wpack->command = SB_COMMAND_DB_DATA;
	// don't need to call ReleaseBuffer here, since we're not changing
	// the real size size
}

void Socket::CheckSequence(const Data &seq)
{
	if( m_socket == 0 ) {
		// don't do any sequence checking on socket 0
		return;
	}

	MAKE_PACKET(spack, seq);
	if( (unsigned int) seq.GetSize() < SB_SEQUENCE_PACKET_SIZE ) {
		eout("Short sequence packet:\n" << seq);
		throw SBError("Socket: invalid sequence packet");
	}

	// we'll cheat here... if the packet's sequence is 0, we'll
	// silently restart, otherwise, fail
	uint32_t sequenceId = spack->data.simple.data.sequence.sequenceId;
	if( sequenceId == 0 ) {
		// silently restart (will advance below)
		m_sequenceId = 0;
	}
	else {
		if( sequenceId != m_sequenceId ) {
			eout("Socket sequence: " << m_sequenceId
				<< ". Packet sequence: " << sequenceId);
			throw SBError("Socket: out of sequence");
		}
	}

	// advance!
	m_sequenceId++;
}

// sends the send packet down to the device, fragmenting if
// necessary, and returns the response in receive, defragmenting
// if needed
// Blocks until response received or timed out in Usb::Device
bool Socket::Packet(const Data &send, Data &receive)
{
	MAKE_PACKET(spack, send);
	if( send.GetSize() < MIN_PACKET_SIZE ||
	    (spack->command != SB_COMMAND_DB_DATA &&
	     spack->command != SB_COMMAND_DB_DONE) )
	{
		// we don't do that around here
		throw std::logic_error("Socket: unknown send data in Packet()");
	}

	if( send.GetSize() > MAX_PACKET_SIZE ) {
		// not yet implemented
		throw std::logic_error("Socket: fragmented sends not implemented");
	}

	Data inFrag;
	receive.Zap();

	// send command
	if( !Send(send, inFrag) )
		return false;

	bool done = false, frag = false;
	while( !done ) {
		MAKE_PACKET(rpack, inFrag);

		// check the packet's validity
		CheckSize(inFrag);

		switch( rpack->command )
		{
		case SB_COMMAND_SEQUENCE_HANDSHAKE:
			CheckSequence(inFrag);
			break;

		case SB_COMMAND_DB_DATA:
			if( frag ) {
				AppendFragment(receive, inFrag);
			}
			else {
				receive = inFrag;
			}
			done = true;
			break;

		case SB_COMMAND_DB_FRAGMENTED:
			AppendFragment(receive, inFrag);
			frag = true;
			break;

		case SB_COMMAND_DB_DONE:
			receive = inFrag;
			done = true;
			break;

		default:
			eout("Command: " << rpack->command << inFrag);
			throw SBError("Socket: unhandled packet in Packet()");
			break;
		}

		if( !done ) {
			// not done yet, ask for another read
			if( !Receive(inFrag) )
				return false;
		}
	}

	return true;
}


} // namespace Syncberry

