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
	m_sequenceId(0)
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

void Socket::Open(uint16_t socket)
{
	if( m_socket != 0 ) {
		// already open
		throw SBError("Socket: already open");
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
	m_dev.TrackBulkRead(m_readEp, receive);
	IO wr = m_dev.ABulkWrite(m_writeEp, send);

	// wait for response
	IO rd = m_dev.PollCompletions();
	while( !rd.IsValid() )
		rd = m_dev.PollCompletions();
	m_lastStatus = rd.GetStatus();
	return m_lastStatus >= 0;
}

bool Socket::Receive(Data &receive)
{
	m_dev.TrackBulkRead(m_readEp, receive);
	IO rd = m_dev.PollCompletions();
	while( !rd.IsValid() )
		rd = m_dev.PollCompletions();
	m_lastStatus = rd.GetStatus();
	return m_lastStatus >= 0;
}

// appends fragment to whole... if whole is empty, simply copies, and
// sets command to DATA instead of FRAGMENTED.  Always updates the
// packet size of whole, to reflect the total size
void Socket::AppendFragment(Data &whole, const Data &fragment)
{
	using Syncberry::Packet;

	if( whole.GetSize() == 0 ) {
		// empty, so just copy
		whole = fragment;
	}
	else {
		// has some data already, so just append
		int size = whole.GetSize();
		unsigned char *buf = whole.GetBuffer(size + fragment.GetSize());
		const Packet *fpack = (const Packet *) fragment.GetData();
		int fragsize = fragment.GetSize() - SB_FRAG_HEADER_SIZE;

		memcpy(buf+size, &fpack->data.param.data.raw, fragsize);
		whole.ReleaseBuffer(size + fragsize);
	}

	// update whole's size and command type for future sanity
	Packet *wpack = (Packet *) whole.GetBuffer();
	wpack->size = (uint16_t) whole.GetSize();
	wpack->command = SB_COMMAND_DB_DATA;
	// don't need to call ReleaseBuffer here, since we're not changing
	// the real size size
}

void Socket::CheckSequence(const Data &seq)
{
	using Syncberry::Packet;
	const Packet *spack = (const Packet *) seq.GetData();
	if( (unsigned int) seq.GetSize() < SB_SEQUENCE_PACKET_SIZE ) {
		dout("Short sequence packet: " << seq);
		throw SBError("Socket: invalid sequence packet");
	}

	uint32_t sequenceId = spack->data.simple.data.sequence.sequenceId;
	if( sequenceId != m_sequenceId ) {
		dout("Socket sequence: " << m_sequenceId
			<< ". Packet sequence: " << sequenceId);
		throw SBError("Socket: out of sequence");
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
	using Syncberry::Packet;

	const Packet *spack = (const Packet *) send.GetData();
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
		const Packet *rpack = (const Packet *) inFrag.GetData();

		// check the packet's validity
		if( inFrag.GetSize() < MIN_PACKET_SIZE ||
		    rpack->size != (unsigned int) inFrag.GetSize() )
		{
			throw SBError("Socket: invalid fragment in Packet");
		}

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
			dout("Command: " << rpack->command << inFrag);
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

