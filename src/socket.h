///
/// \file	socket.h
///		Class wrapper to encapsulate the Blackberry USB logical socket
///

#ifndef __BARRY_SOCKET_H__
#define __BARRY_SOCKET_H__

#include <stdint.h>

// forward declarations
class Data;
namespace Usb { class Device; }

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
	uint8_t m_flag;
	uint32_t m_sequenceId;

	int m_lastStatus;

private:
	// sends 'send' data to device, and waits for response, using
	// "read first, write second" order observed in capture
	void AppendFragment(Data &whole, const Data &fragment);
	void CheckSequence(const Data &seq);

public:
	Socket(Usb::Device &dev, int writeEndpoint, int readEndpoint);
	~Socket();

	int GetLastStatus() const { return m_lastStatus; }
	uint16_t GetSocket() const { return m_socket; }

	void Open(uint16_t socket, uint8_t flag);
	void Close();

	// Send and Receive are available before Open...
	// an unopened socket defaults to socket 0, which you need
	// in order to set the blackberry mode
	bool Send(const Data &send, Data &receive);
	bool Receive(Data &receive);

	// sends the send packet down to the device, fragmenting if
	// necessary, and returns the response in receive, defragmenting
	// if needed
	// Blocks until response received or timed out in Usb::Device
	bool Packet(const Data &send, Data &receive);

	// some handy wrappers for the Packet() interface
	bool NextRecord(Data &receive);
};


} // namespace Barry

#endif

