///
/// \file	controller.cc
///		High level Barry API class
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

#include "controller.h"
#include "common.h"
#include "protocol.h"
#include "protostructs.h"
#include "error.h"
#include "data.h"
#include "parser.h"
#include "builder.h"
#include "endian.h"
#include "packet.h"

#define __DEBUG_MODE__
#include "debug.h"

#include <sstream>

#include <iomanip>

namespace Barry {

//
// Controller constructor
//
/// Constructor for the Controller class.  Requires a valid ProbeResult
/// object to find the USB device to talk to.
///
/// \param[in]	device		One of the ProbeResult objects from the
///				Probe class.
///
Controller::Controller(const ProbeResult &device)
	: m_dev(device.m_dev),
	m_iface(0),
	m_pin(device.m_pin),
	m_socket(m_dev, device.m_ep.write, device.m_ep.read, device.m_zeroSocketSequence),
	m_mode(Unspecified),
	m_modeSocket(0),
	m_halfOpen(false)
{
	unsigned char cfg;
	if( !m_dev.GetConfiguration(cfg) )
		throw Usb::Error(m_dev.GetLastError(),
			"Controller: GetConfiguration failed");

	if( cfg != BLACKBERRY_CONFIGURATION ) {
		if( !m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
			throw Usb::Error(m_dev.GetLastError(),
				"Controller: SetConfiguration failed");
	}

	m_iface = new Usb::Interface(m_dev, device.m_interface);
}

Controller::~Controller()
{
	// trap exceptions in the destructor
	try {
		// a non-default socket has been opened, close it
		m_socket.Close();
	}
	catch( std::runtime_error &re ) {
		// do nothing... log it?
		dout("Exception caught in ~Socket: " << re.what());
	}

	// cleanup the interface
	delete m_iface;

	// this happens when for some reason the Desktop mode
	// is not fully opened, but the device has already recommended
	// a socket to open... in this case, reset the device
	// in the hopes that on next open, it will be in a
	// recognizable state.
	//
	// FIXME - this should not be necessary, and someday we
	// we should figure out how to handle the "already open"
	// response we get for the Desktop
	//
	if( m_halfOpen ) {
		dout("Controller object destroyed in halfopen state, resetting device");
		m_dev.Reset();
	}
}

///////////////////////////////////////////////////////////////////////////////
// protected members

void Controller::SelectMode(ModeType mode)
{
	// start fresh
	m_modeSocket = 0;

	// select mode
	Protocol::Packet packet;
	packet.socket = 0;
	packet.size = htobs(SB_MODE_PACKET_COMMAND_SIZE);
	packet.command = SB_COMMAND_SELECT_MODE;
	packet.u.socket.socket = htobs(SB_MODE_REQUEST_SOCKET);
	packet.u.socket.sequence = 0; // updated by Socket::Send()
	memset(packet.u.socket.u.mode.name, 0, sizeof(packet.u.socket.u.mode.name));

	char *modeName = (char *) packet.u.socket.u.mode.name;
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

	case UsbSerData:
		strcpy(modeName, "RIM_UsbSerData");
		break;

	default:
		throw std::logic_error("Controller: Invalid mode in SelectMode");
		break;
	}

	// send mode command before we open, as a default socket is socket 0
	Data command(&packet, btohs(packet.size));
	Data response;

	try {
		m_socket.Send(command, response);

		// get the data socket number
		// indicates the socket number that
		// should be used below in the Open() call
		Protocol::CheckSize(response, SB_MODE_PACKET_RESPONSE_SIZE);
		MAKE_PACKET(modepack, response);
		if( modepack->command != SB_COMMAND_MODE_SELECTED ) {
			eeout(command, response);
			throw Error("Controller: mode not selected");
		}

		// save the socket that the device is expecting us to use
		m_modeSocket = btohs(modepack->u.socket.socket);
	}
	catch( Usb::Error & ) {
		eout("Controller: error setting desktop mode");
		eeout(command, response);
		throw;
	}
}

unsigned int Controller::GetCommand(CommandType ct)
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
		throw std::logic_error("Controller: unknown command type");
	}

	if( cmd == 0 ) {
		std::ostringstream oss;
		oss << "Controller: unable to get command code: " << cmdName;
		throw Error(oss.str());
	}

	return cmd;
}

void Controller::LoadCommandTable()
{
	assert( m_mode == Desktop );

	char rawCommand[] = { 6, 0, 0x0a, 0, 0x40, 0, 0, 1, 0, 0 };
	*((uint16_t*) rawCommand) = htobs(m_socket.GetSocket());

	Data command(rawCommand, sizeof(rawCommand));
	Data response;

	try {
		m_socket.Packet(command, response);

		MAKE_PACKET(rpack, response);
		while( rpack->command != SB_COMMAND_DB_DONE ) {
			m_socket.NextRecord(response);

			rpack = (const Protocol::Packet *) response.GetData();
			if( rpack->command == SB_COMMAND_DB_DATA && btohs(rpack->size) > 10 ) {
				// second packet is generally large, and contains
				// the command table
				m_commandTable.Clear();
				m_commandTable.Parse(response, 6);
			}
		}

		ddout(m_commandTable);

	}
	catch( Usb::Error & ) {
		eout("Controller: error getting command table");
		eeout(command, response);
		throw;
	}
}

void Controller::LoadDBDB()
{
	assert( m_mode == Desktop );

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.GetDBDB();

	m_socket.Packet(packet);

	while( packet.Command() != SB_COMMAND_DB_DONE ) {
		if( packet.Command() == SB_COMMAND_DB_DATA ) {
			m_dbdb.Clear();
			m_dbdb.Parse(response);
		}

		// advance!
		m_socket.NextRecord(response);
	}
}


///////////////////////////////////////////////////////////////////////////////
// public API

//
// GetDBID
//
/// Get numeric database ID by name.
///
/// \param[in]	name		Name of database, which matches one of the
///				names listed in GetDBDB()
///
/// \exception	Barry::Error
///		Thrown if name not found.
///
unsigned int Controller::GetDBID(const std::string &name) const
{
	unsigned int ID = 0;
	// FIXME - this needs a better error handler...
	if( !m_dbdb.GetDBNumber(name, ID) ) {
		throw Error("Controller: database name not found: " + name);
	}
	return ID;
}

//
// OpenMode
//
/// Select device mode.  This is required before using any other mode-based
/// operations, such as GetDBDB() and LoadDatabase().  Currently only
/// Desktop mode is supported, but the following modes are available.
/// (See ModeType)
///
///	- Controller::Bypass
///	- Controller::Desktop
///	- Controller::JavaLoader
///
/// This function opens a socket to the device when in Desktop mode.
/// If the device requires it, specify the password with a conat char*
/// string in password.  The password will not be stored in memory
/// inside this class, only a hash will be generated from it.  After
/// using the hash, the hash memory will be set to 0.  The application
/// is responsible for safely handling the raw password data.
///
/// You can retry the password by catching Barry::BadPassword and
/// calling RetryPassword() with the new password.
///
/// \exception	Barry::Error
///		Thrown on protocol error.
///
/// \exception	std::logic_error()
///		Thrown if unsupported mode is requested, or if socket
///		already open.
///
/// \exception	Barry::BadPassword
///		Thrown when password is invalid or if not enough retries
///		left in the device.
///
void Controller::OpenMode(ModeType mode, const char *password)
{
	if( m_mode != mode ) {
		m_socket.Close();
		SelectMode(mode);
		m_mode = mode;

		RetryPassword(password);
	}
}

//
// RetryPassword
//
/// Retry a failed password attempt from the first call to OpenMode().
/// Only call this function on Barry::BadPassword exceptions thrown
/// from OpenMode().
///
/// \exception	Barry::Error
///		Thrown on protocol error.
///
/// \exception	std::logic_error()
///		Thrown if in unsupported mode, or if socket already open.
///
/// \exception	Barry::BadPassword
///		Thrown when password is invalid or if not enough retries
///		left in the device.
///
void Controller::RetryPassword(const char *password)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in RetryPassword");

	if( m_socket.GetSocket() != 0 )
		throw std::logic_error("Socket alreay open in RetryPassword");

	m_halfOpen = true;
	m_socket.Open(m_modeSocket, password);
	m_halfOpen = false;

	switch( m_mode )
	{
	case Desktop:
		// get command table
		LoadCommandTable();

		// get database database
		LoadDBDB();
		break;

	case UsbSerData:
		// nothing to do
		break;

	default:
		throw std::logic_error("Mode not implemented");
	}
}

//
// GetRecordStateTable
//
/// Retrieve the record state table from the handheld device, using the given
/// database ID.  Results will be stored in result, which will be cleared
/// before adding.
///
void Controller::GetRecordStateTable(unsigned int dbId, RecordStateTable &result)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in GetRecordStateTable");

	dout("Database ID: " << dbId);

	// start fresh
	result.Clear();

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.GetRecordStateTable(dbId);

	m_socket.Packet(packet);
	result.Parse(response);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket.NextRecord(response);
}

//
// AddRecord
//
/// Adds a record to the specified database.  RecordId is
/// retrieved from build, and duplicate IDs are allowed by the device
/// (i.e. you can have two records with the same ID) 
/// but *not* recommended!
//
void Controller::AddRecord(unsigned int dbId, Builder &build)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in GetRecord");

	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);

	if( packet.SetRecord(dbId, build) ) {

		std::ostringstream oss;

		m_socket.Packet(packet);

		// successful packet transfer, so check the network return code
		if( packet.Command() != SB_COMMAND_DB_DONE ) {
			oss << "Controller: device responded with unexpected packet command code: "
			    << "0x" << std::hex << packet.Command();
			throw Error(oss.str());
		}

		if( packet.ReturnCode() != 0 ) {
			oss << "Controller: device responded with error code (command: "
			    << packet.Command() << ", code: "
			    << packet.ReturnCode() << ")";
			throw Error(oss.str());
		}
	}
}

//
// GetRecord
//
/// Retrieves a specific record from the specified database.
/// The stateTableIndex comes from the GetRecordStateTable()
/// function.  GetRecord() does not clear the dirty flag.
///
void Controller::GetRecord(unsigned int dbId,
			   unsigned int stateTableIndex,
			   Parser &parser)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in GetRecord");

	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.GetRecordByIndex(dbId, stateTableIndex);

	m_socket.Packet(packet);

	// perform copious packet checks
	if( response.GetSize() < SB_PACKET_RESPONSE_HEADER_SIZE ) {
		eeout(command, response);

		std::ostringstream oss;
		oss << "Controller: invalid response packet size of "
		    << std::dec << response.GetSize();
		eout(oss.str());
		throw Error(oss.str());
	}
	if( packet.Command() != SB_COMMAND_DB_DATA ) {
		eeout(command, response);

		std::ostringstream oss;
		oss << "Controller: unexpected command of 0x"
		    << std::setbase(16) << packet.Command()
		    << " instead of expected 0x"
		    << std::setbase(16) << (unsigned int)SB_COMMAND_DB_DATA;
		eout(oss.str());
		throw Error(oss.str());
	}

	// grab that data
	packet.Parse(parser);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket.NextRecord(response);
}

//
// SetRecord
//
/// Overwrites a specific record in the device as identified by the
/// stateTableIndex.
///
void Controller::SetRecord(unsigned int dbId, unsigned int stateTableIndex,
			   Builder &build)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in SetRecord");

	dout("Database ID: " << dbId << " Index: " << stateTableIndex);

	Data command, response;
	DBPacket packet(*this, command, response);

	// loop until builder object has no more data
	if( !packet.SetRecordByIndex(dbId, stateTableIndex, build) ) {
		throw std::logic_error("Controller: no data available in SetRecord");
	}

	m_socket.Packet(packet);

	std::ostringstream oss;

	// successful packet transfer, so check the network return code
	if( packet.Command() != SB_COMMAND_DB_DONE ) {
		oss << "Controller: device responded with unexpected packet command code: "
		    << "0x" << std::hex << packet.Command();
		throw Error(oss.str());
	}

	if( packet.ReturnCode() != 0 ) {
		oss << "Controller: device responded with error code (command: "
		    << packet.Command() << ", code: "
		    << packet.ReturnCode() << ")";
		throw Error(oss.str());
	}
}

//
// ClearDirty
//
/// Clears the dirty flag on the specified record in the specified database.
///
void Controller::ClearDirty(unsigned int dbId, unsigned int stateTableIndex)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in ClearDirty");

	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.SetRecordFlags(dbId, stateTableIndex, 0);

	m_socket.Packet(packet);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket.NextRecord(response);
}

//
// DeleteRecord
//
/// Deletes the specified record in the specified database.
///
void Controller::DeleteRecord(unsigned int dbId, unsigned int stateTableIndex)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in DeleteRecord");

	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.DeleteRecordByIndex(dbId, stateTableIndex);

	m_socket.Packet(packet);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket.NextRecord(response);
}

//
// LoadDatabase
//
/// Retrieve a database from the handheld device, using the given parser
/// to parse the resulting data, and optionally store it.
///
/// See the RecordParser<> template to create a parser object.  The
/// RecordParser<> template allows custom storage based on the type of
/// database record retrieved.  The database ID and the parser Record
/// type must match.
///
/// \param[in]	dbId		Database Database ID - use GetDBID()
/// \param[out]	parser		Parser object which parses the resulting
///				protocol data, and optionally stores it in
///				a custom fashion.  See the RecordParser<>
///				template.
///
/// \exception	Barry::Error
///		Thrown on protocol error.
///
/// \exception	std::logic_error
///		Thrown if not in Desktop mode.
///
void Controller::LoadDatabase(unsigned int dbId, Parser &parser)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in LoadDatabase");

	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.GetRecords(dbId);

	m_socket.Packet(packet);

	while( packet.Command() != SB_COMMAND_DB_DONE ) {
		if( packet.Command() == SB_COMMAND_DB_DATA ) {
			// this size is the old header size, since using
			// old command above
			packet.Parse(parser);
		}

		// advance!
		m_socket.NextRecord(response);
	}
}

void Controller::SaveDatabase(unsigned int dbId, Builder &builder)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in SaveDatabase");

	dout("Database ID: " << dbId);

	// Protocol note: so far in testing, this CLEAR_DATABASE operation is
	//                required, since every record sent via SET_RECORD
	//                is treated like a hypothetical "ADD_RECORD" (perhaps
	//                SET_RECORD should be renamed)... I don't know if
	//                there is a real SET_RECORD... all I know is from
	//                the Windows USB captures, which uses this same
	//                technique.
	Data command, response;
	DBPacket packet(*this, command, response);
	packet.ClearDatabase(dbId);

	// wait up to a minute here for old, slower devices with lots of data
	m_socket.Packet(packet, 60000);
	if( packet.ReturnCode() != 0 ) {
		std::ostringstream oss;
		oss << "Controller: could not clear database: (command: "
		    << "0x" << std::hex << packet.Command() << ", code: "
		    << "0x" << std::hex << packet.ReturnCode() << ")";
		throw Error(oss.str());
	}

	// check response to clear command was successful
	if( packet.Command() != SB_COMMAND_DB_DONE ) {
		eeout(command, response);
		throw Error("Controller: error clearing database, bad response");
	}

	// loop until builder object has no more data
	bool first = true;
	while( packet.SetRecord(dbId, builder) ) {
		dout("Database ID: " << dbId);

		m_socket.Packet(packet, first ? 60000 : -1);
		first = false;

		std::ostringstream oss;
		// successful packet transfer, so check the network return code
		if( packet.Command() != SB_COMMAND_DB_DONE ) {
			oss << "Controller: device responded with unexpected packet command code: "
			    << "0x" << std::hex << packet.Command();
			throw Error(oss.str());
		}

		if( packet.ReturnCode() != 0 ) {
			oss << "Controller: device responded with error code (command: "
			    << packet.Command() << ", code: "
			    << packet.ReturnCode() << ")";
			throw Error(oss.str());
		}
	}
}



//////////////////////////////////////////////////////////////////////////////
// UsbSerData mode - modem specific

// can be called from separate thread
void Controller::SerialRead(Data &data, int timeout)
{
	if( m_mode != UsbSerData )
		throw std::logic_error("Wrong mode in SerialRead");

	m_socket.Receive(data, timeout);
}

// based on Rick Scott's XmBlackBerry's serdata.c
void Controller::SerialWrite(const Data &data)
{
	if( m_mode != UsbSerData )
		throw std::logic_error("Wrong mode in SerialWrite");

	if( data.GetSize() <= 0 )
		return;	// nothing to do

	int size = data.GetSize() + 4;
	unsigned char *buf = m_writeCache.GetBuffer(size);
	MAKE_PACKETPTR_BUF(spack, buf);

	// copy data over to cache packet
	memcpy(&buf[4], data.GetData(), data.GetSize());

	// setup header
	spack->socket = htobs(m_socket.GetSocket());
	spack->size = htobs(size);

	// release and send
	m_writeCache.ReleaseBuffer(size);
	m_socket.Send(m_writeCache);

/*
	unsigned char buf[0x400];
	int num_read;
	int i;

	//
	// This is pretty ugly, but I have to put the HDLC flags into
	// the packets. RIM seems to need flags around every frame, and
	// a flag _cannot_ be an end and a start flag.
	//
	for (i = 0; i < num_read; i++) {
		BufferAdd(&serdata->data, &buf[i], 1);
		if (BufferData(&serdata->data)[0] == 0x7e && buf[i] == 0x7e) {
			if (BufferLen(&serdata->data) > 1 &&
				BufferData(&serdata->data)[0] == 0x7e && 
				BufferData(&serdata->data)[1] == 0x7e)
			{
				BufferPullHead(&serdata->data, 1);
			}
			else
			{
			}
			if (BufferLen(&serdata->data) > 2)
			{
				if ((BufferLen(&serdata->data) + 4) % 16 == 0)
				{
					BufferAdd(&serdata->data, (unsigned char *)"\0", 1);
				}
				send_packet(serdata, BufferData(&serdata->data), BufferLen(&serdata->data));
				BufferEmpty(&serdata->data);
				BufferAdd(&serdata->data, (unsigned char *)"\176", 1);
			}
			if (BufferLen(&serdata->data) == 2)
			{
				BufferPullTail(&serdata->data, 1);
			}
			else
			{
			}
		}
		else
		{
		}
	}
	if (BufferData(&serdata->data)[0] == 0x7e &&
	   memcmp(&BufferData(&serdata->data)[1], "AT", 2) == 0)
	{
		BufferPullHead(&serdata->data, 1);
	}
	if (BufferData(&serdata->data)[0] != 0x7e)
	{
		debug(9, "%s:%s(%d) - %i\n",
			__FILE__, __FUNCTION__, __LINE__,
			BufferLen(&serdata->data));
		send_packet(serdata, BufferData(&serdata->data), BufferLen(&serdata->data));
		BufferEmpty(&serdata->data);
	}
*/
}


} // namespace Barry

