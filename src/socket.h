///
/// \file	socket.h
///		Class wrapper to encapsulate the Blackberry USB logical socket
///

#ifndef __SYNCBERRY_SOCKET_H__
#define __SYNCBERRY_SOCKET_H__

#include <stdint.h>

// forward declarations
class Data;
namespace Usb { class Device; }

namespace Syncberry {

class Socket
{
	Usb::Device &m_dev;
	int m_writeEp, m_readEp;
	uint16_t m_socket;		// defaults to 0, which is valid,
					// since socket 0 is always open
					// If this is not 0, then class will
					// deal with closing automatically.
	uint32_t m_sequenceId;

	int m_lastStatus;

private:
	// sends 'send' data to device, and waits for response, using
	// "read first, write second" order observed in capture
	bool Send(const Data &send, Data &receive);
	bool Receive(Data &receive);
	void AppendFragment(Data &whole, const Data &fragment);
	void CheckSequence(const Data &seq);

public:
	Socket(Usb::Device &dev, int writeEndpoint, int readEndpoint);
	~Socket();

	int GetLastStatus() const { return m_lastStatus; }

	void Open(uint16_t socket);
	void Close();

	// sends the send packet down to the device, fragmenting if
	// necessary, and returns the response in receive, defragmenting
	// if needed
	// Blocks until response received or timed out in Usb::Device
	bool Packet(const Data &send, Data &receive);
};


} // namespace Syncberry

#endif

