///
/// \file	blackberry.cc
///		High level BlackBerry API class
///

#include "blackberry.h"
#include "sbcommon.h"
#include "protocol.h"
#include "error.h"
#include "data.h"
#include "debug.h"

namespace Syncberry {

Blackberry::Blackberry(const ProbeResult &device)
	: m_dev(device.m_dev),
	m_iface(m_dev, BLACKBERRY_INTERFACE),
	m_pin(device.m_pin),
	m_socket(m_dev, WRITE_ENDPOINT, READ_ENDPOINT)
{
	if( !m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
		throw SBError(m_dev.GetLastError(),
			"Blackberry: SetConfiguration failed");
}

Blackberry::~Blackberry()
{
}

void Blackberry::Test()
{
	Packet packet;
	packet.socket = 0;
	packet.size = SB_MODE_PACKET_SIZE;
	packet.command = SB_COMMAND_SELECT_MODE;
	packet.data.param.param = SB_PARAM_DEFAULT;
	packet.data.param.data.mode.unknown1 = 0;
	packet.data.param.data.mode.unknown2 = 0x05;	// FIXME
	memcpy(packet.data.param.data.mode.mode_name, "RIM Desktop\0\0\0\0\0",
		sizeof(packet.data.param.data.mode.mode_name));

	// send mode command before we open, as a default socket is socket 0
	Data command(&packet, packet.size);
	Data response;
	if( !m_socket.Send(command, response) ) {
		eout("Sent packet:\n" << command);
		eout("Response packet:\n" << response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error setting desktop mode");
	}

	// now open our database communication socket
	m_socket.Open(SB_SOCKET_COMM, SB_SOCKET_INIT_PARAM);
}

} // namespace Syncberry

/*

This is the conversation we are aiming for:

The numbers at the end of this command change... perhaps a date?
They are always returned.
sep: 5
    00000000: 00 00 10 00 01 ff 00 00 a8 18 da 8d 6c 02 00 00

rep: 82
    00000000: 00 00 10 00 02 ff 00 00 a8 18 da 8d 6c 02 00 00

This command is always the same.
sep: 5
    00000000: 00 00 0c 00 05 ff 00 01 14 00 01 00

rep: 82
    00000000: 00 00 20 00 06 ff 00 01 14 00 01 00 51 e1 33 6b
    00000010: f3 09 bc 37 3b a3 5e ed ff 30 a1 3a 60 c9 81 8e

This command is always the same.  It retrieves the PIN number.
sep: 5
    00000000: 00 00 0c 00 05 ff 00 02 08 00 04 00

rep: 82
    00000000: 00 00 14 00 06 ff 00 02 08 00 04 00 04 00 00 00
    00000010: e3 ef 09 30


Note: There are about 3 different top level modes that this command activates.
	RIM Bypass, RIM Desktop, and RIM_Javasomething
	We want RIM Desktop, as all the data is transferred in this mode.
sep: 5
    00000000: 00 00 18 00 07 ff 00 05 52 49 4d 20 44 65 73 6b  ........RIM Desk
    00000010: 74 6f 70 00 00 00 00 00                          top.....
rep: 82
    00000000: 00 00 2c 00 08 06 00 05 52 49 4d 20 44 65 73 6b  ..,.....RIM Desk
    00000010: 74 6f 70 00 00 00 00 00 00 00 00 00 01 00 04 00  top.............
    00000020: 02 00 04 00 03 01 00 00 04 01 00 00              ............

Open a socket...
sep: 5
    00000000: 00 00 08 00 0a 06 00 06

rep: 82
    00000000: 00 00 08 00 10 06 00 06


Get the Command Table...
sep: 5
    00000000: 06 00 0a 00 40 00 00 01 00 00
rep: 82
    00000000: 00 00 0c 00 13 06 01 00 01 00 00 00
rep: 82
    00000000: 06 00 0a 00 40 00 00 02 00 04


Continue Get Command Table...
sep: 5
    00000000: 06 00 07 00 41 00 00
rep: 82
    00000000: 00 00 0c 00 13 06 01 00 02 00 00 00
rep: 82
    00000000: 06 00 35 00 40 00 0c 01 53 65 72 76 69 63 65 20
    00000010: 42 6f 6f 6b 0e 02 44 65 76 69 63 65 20 4f 70 74
    00000020: 69 6f 6e 73 0f 03 44 61 74 61 62 61 73 65 20 41
    00000030: 63 63 65 73 73


At this point, we are well into the database access conversation... the
next thing to do is retrieve the Database Database, and then use that
to get the item count and all the records in each database.
	Memos
	Calendar
	Contacts
	Email


*/

