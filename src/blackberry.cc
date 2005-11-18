///
/// \file	blackberry.cc
///		High level BlackBerry API class
///

#include "blackberry.h"
#include "sbcommon.h"
#include "protocol.h"
#include "error.h"
#include "data.h"

#define __DEBUG_MODE__
#include "debug.h"

#include <sstream>

#include <iomanip>

namespace Syncberry {

Blackberry::Blackberry(const ProbeResult &device)
	: m_dev(device.m_dev),
	m_iface(m_dev, BLACKBERRY_INTERFACE),
	m_pin(device.m_pin),
	m_socket(m_dev, WRITE_ENDPOINT, READ_ENDPOINT),
	m_mode(Unspecified)
{
	if( !m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
		throw SBError(m_dev.GetLastError(),
			"Blackberry: SetConfiguration failed");
}

Blackberry::~Blackberry()
{
}

void Blackberry::SelectMode(ModeType mode, uint16_t &socket, uint8_t &flag)
{
	// select mode
	Packet packet;
	packet.socket = 0;
	packet.size = SB_MODE_PACKET_COMMAND_SIZE;
	packet.command = SB_COMMAND_SELECT_MODE;
	packet.data.simple.data.mode.socket = SB_MODE_REQUEST_SOCKET;
	packet.data.simple.data.mode.flag = 0x05;	// FIXME
	memset(packet.data.simple.data.mode.modeName, 0,
		sizeof(packet.data.simple.data.mode.modeName));

	char *modeName = (char *) packet.data.simple.data.mode.modeName;
	switch( mode )
	{
	case Bypass:
		strcpy(modeName, "RIM Bypass");
		break;

	case Desktop:
		strcpy(modeName, "RIM Desktop");
		break;

	case JavaLoader:
		strcpy(modeName, "RIM_JavaLoader");
		break;

	default:
		throw std::logic_error("Blackberry: Invalid mode in SelectMode");
		break;
	}

	// send mode command before we open, as a default socket is socket 0
	Data command(&packet, packet.size);
	Data response;
	if( !m_socket.Send(command, response) ) {
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error setting desktop mode");
	}

	// get the data socket number
	// indicates the socket number that
	// should be used below in the Open() call
	CheckSize(response, SB_MODE_PACKET_RESPONSE_SIZE);
	MAKE_PACKET(modepack, response);
	if( modepack->command != SB_COMMAND_MODE_SELECTED ) {
		eeout(command, response);
		throw SBError("Blackberry: mode not selected");
	}

	// return the socket and flag that the device is expecting us to use
	socket = modepack->data.simple.data.mode.socket;
	flag = modepack->data.simple.data.mode.flag + 1;
}

void Blackberry::OpenMode(ModeType mode)
{
	uint16_t socket;
	uint8_t flag;

	m_socket.Close();
	SelectMode(mode, socket, flag);
	m_socket.Open(socket, flag);
}

unsigned int Blackberry::GetCommand(CommandType ct)
{
	unsigned int cmd = 0;
	char *cmdName = "Unknown";

	switch( ct )
	{
	case DatabaseAccess:
		cmdName = "Database Access";
		cmd = m_commandTable.GetCommand(cmdName);
		break;
	default:
		throw std::logic_error("Blackberry: unknown command type");
	}

	if( cmd == 0 ) {
		std::ostringstream oss;
		oss << "Blackberry: unable to get command code: " << cmdName;
		throw SBError(oss.str());
	}

	return cmd;
}

void Blackberry::Test()
{
	// open desktop mode socket
	OpenMode(Desktop);

	// get command table
	GetCommandTable();

	// get database database
	GetDBDB();

	// get contact records
	GetAddressBook();

	// cleanup... not strictly needed, but nice to "release" the device
	m_socket.Close();
}

void Blackberry::GetCommandTable()
{
	char rawCommand[] = { 6, 0, 0x0a, 0, 0x40, 0, 0, 1, 0, 0 };
	*((uint16_t*) rawCommand) = m_socket.GetSocket();

	Data command(rawCommand, sizeof(rawCommand));
	Data response;
	if( !m_socket.Packet(command, response) ) {
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error getting command table");
	}

	MAKE_PACKET(firstpack, response);
	while( firstpack->command != SB_COMMAND_DB_DONE ) {
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw SBError(m_socket.GetLastStatus(),
				"Blackberry: error getting command table(next)");
		}

		MAKE_PACKET(rpack, response);
		if( rpack->command == SB_COMMAND_DB_DATA && rpack->size > 10 ) {
			// second packet is generally large, and contains
			// the command table
			m_commandTable.Clear();
			m_commandTable.Parse(response, 6);
		}
	}

	ddout(m_commandTable);
}

void Blackberry::GetDBDB()
{
	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 7;
	packet.command = SB_COMMAND_DB_DATA;
	packet.data.param.param = GetCommand(DatabaseAccess);
	packet.data.param.data.db.command = SB_DBOP_GET_DBDB;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error getting database database");
	}

	MAKE_PACKET(rpack, response);
	while( rpack->command != SB_COMMAND_DB_DONE ) {
		if( rpack->command == SB_COMMAND_DB_DATA ) {
			m_dbdb.Clear();
			m_dbdb.Parse(response, 12);	// FIXME - hardcoded
		}

		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw SBError(m_socket.GetLastStatus(),
				"Blackberry: error getting command table(next)");
		}
		rpack = (const Packet *) response.GetData();
	}

	ddout(m_dbdb);
}

void Blackberry::GetAddressBook()
{
// just dumping to screen for now

	unsigned int ABID = m_dbdb.GetDBNumber("Address Book");
	// FIXME - this needs a better error handler... the dbdb needs one too!
	if( ABID == 0 ) {
		throw SBError("Blackberry: Address Book not found");
	}

	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 9;
	packet.command = SB_COMMAND_DB_DATA;
	packet.data.param.param = GetCommand(DatabaseAccess);
	packet.data.param.data.db.command = SB_DBOP_GET_RECORDS;
	packet.data.param.data.db.databaseId = ABID;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error getting database database");
	}

int count = 0;
	MAKE_PACKET(rpack, response);
	while( rpack->command != SB_COMMAND_DB_DONE ) {
		if( rpack->command == SB_COMMAND_DB_DATA ) {
			Contact contact;
			contact.Parse(response, 13);	// FIXME - hardcoded

			// FIXME - do something better with the data
			count++;
			std::cout << contact;
		}

		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw SBError(m_socket.GetLastStatus(),
				"Blackberry: error getting command table(next)");
		}
		rpack = (const Packet *) response.GetData();
	}

std::cout << "Contact count: " << std::setbase(10) << count;
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

