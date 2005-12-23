///
/// \file	controller.cc
///		High level Barry API class
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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
	m_iface(m_dev, BLACKBERRY_INTERFACE),
	m_pin(device.m_pin),
	m_socket(m_dev, WRITE_ENDPOINT, READ_ENDPOINT),
	m_mode(Unspecified)
{
	if( !m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
		throw BError(m_dev.GetLastError(),
			"Controller: SetConfiguration failed");
}

Controller::~Controller()
{
}

///////////////////////////////////////////////////////////////////////////////
// protected members

void Controller::SelectMode(ModeType mode, uint16_t &socket, uint8_t &flag)
{
	// select mode
	Packet packet;
	packet.socket = 0;
	packet.size = SB_MODE_PACKET_COMMAND_SIZE;
	packet.command = SB_COMMAND_SELECT_MODE;
	packet.u.mode.socket = SB_MODE_REQUEST_SOCKET;
	packet.u.mode.flag = 0x05;	// FIXME
	memset(packet.u.mode.modeName, 0, sizeof(packet.u.mode.modeName));

	char *modeName = (char *) packet.u.mode.modeName;
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
		throw std::logic_error("Controller: Invalid mode in SelectMode");
		break;
	}

	// send mode command before we open, as a default socket is socket 0
	Data command(&packet, packet.size);
	Data response;
	if( !m_socket.Send(command, response) ) {
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error setting desktop mode");
	}

	// get the data socket number
	// indicates the socket number that
	// should be used below in the Open() call
	CheckSize(response, SB_MODE_PACKET_RESPONSE_SIZE);
	MAKE_PACKET(modepack, response);
	if( modepack->command != SB_COMMAND_MODE_SELECTED ) {
		eeout(command, response);
		throw BError("Controller: mode not selected");
	}

	// return the socket and flag that the device is expecting us to use
	socket = modepack->u.mode.socket;
	flag = modepack->u.mode.flag + 1;
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
		throw BError(oss.str());
	}

	return cmd;
}

void Controller::LoadCommandTable()
{
	assert( m_mode == Desktop );

	char rawCommand[] = { 6, 0, 0x0a, 0, 0x40, 0, 0, 1, 0, 0 };
	*((uint16_t*) rawCommand) = m_socket.GetSocket();

	Data command(rawCommand, sizeof(rawCommand));
	Data response;
	if( !m_socket.Packet(command, response) ) {
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error getting command table");
	}

	MAKE_PACKET(rpack, response);
	while( rpack->command != SB_COMMAND_DB_DONE ) {
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw BError(m_socket.GetLastStatus(),
				"Controller: error getting command table(next)");
		}

		rpack = (const Packet *) response.GetData();
		if( rpack->command == SB_COMMAND_DB_DATA && rpack->size > 10 ) {
			// second packet is generally large, and contains
			// the command table
			m_commandTable.Clear();
			m_commandTable.Parse(response, 6);
		}
	}

	ddout(m_commandTable);
}

void Controller::LoadDBDB()
{
	assert( m_mode == Desktop );

	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 7;
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
//	packet.u.db.u.command.operation = SB_DBOP_GET_DBDB;
	packet.u.db.u.command.operation = SB_DBOP_OLD_GET_DBDB;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error getting database database");
	}

	MAKE_PACKET(rpack, response);
	while( rpack->command != SB_COMMAND_DB_DONE ) {
		if( rpack->command == SB_COMMAND_DB_DATA ) {
			m_dbdb.Clear();
			m_dbdb.Parse(response);
		}

		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw BError(m_socket.GetLastStatus(),
				"Controller: error getting command table(next)");
		}
		rpack = (const Packet *) response.GetData();
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
/// \exception	Barry::BError
///		Thrown if name not found.
///
unsigned int Controller::GetDBID(const std::string &name) const
{
	unsigned int ID = m_dbdb.GetDBNumber(name);
	// FIXME - this needs a better error handler... the dbdb needs one too!
	if( ID == 0 ) {
		throw BError("Controller: database name not found");
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
/// \exception	Barry::BError
///		Thrown on protocol error.
///
/// \exception	std::logic_error()
///		Thrown if unsupported mode is requested.
///
void Controller::OpenMode(ModeType mode)
{
	uint16_t socket;
	uint8_t flag;

	if( m_mode != mode ) {
		m_socket.Close();
		SelectMode(mode, socket, flag);
		m_socket.Open(socket, flag);
		m_mode = mode;

		switch( m_mode )
		{
		case Desktop:
			// get command table
			LoadCommandTable();

			// get database database
			LoadDBDB();
			break;

		default:
			throw std::logic_error("Mode not implemented");
		}
	}
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
/// \exception	Barry::BError
///		Thrown on protocol error.
///
/// \exception	std::logic_error
///		Thrown if not in Desktop mode.
///
void Controller::LoadDatabase(unsigned int dbId, Parser &parser)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in LoadDatabase");

	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 9;
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
//	packet.u.db.u.command.operation = SB_DBOP_GET_RECORDS;
	packet.u.db.u.command.operation = SB_DBOP_OLD_GET_RECORDS;
	packet.u.db.u.command.databaseId = dbId;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eout("Database ID: " << dbId);
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error loading database");
	}

	MAKE_PACKET(rpack, response);
	while( response.GetSize() >= SB_PACKET_HEADER_SIZE &&
	       rpack->command != SB_COMMAND_DB_DONE )
	{
		if( rpack->command == SB_COMMAND_DB_DATA ) {
			// this size is the old header size, since using
			// old command above
			size_t size = SB_PACKET_OLD_RESPONSE_HEADER_SIZE;
			if( response.GetSize() >= size )
				parser(response, size);
		}

		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw BError(m_socket.GetLastStatus(),
				"Controller: error loading database (next)");
		}
		rpack = (const Packet *) response.GetData();
	}
}

void Controller::SaveDatabase(unsigned int dbId, Builder &builder)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in SaveDatabase");

	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 9;
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_CLEAR_DATABASE;
	packet.u.db.u.command.databaseId = dbId;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eout("Database ID: " << dbId);
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error clearing database");
	}

	// check response to clear command was successful
	MAKE_PACKET(rpack, response);
	if( rpack->command != SB_COMMAND_DB_DONE ) {
		throw BError(m_socket.GetLastStatus(),
			"Controller: error clearing database, bad response");
	}

	// loop until builder object has no more data
	while( builder(command, SB_PACKET_UPLOAD_HEADER_SIZE, dbId) ) {
		size_t size = command.GetSize();
		unsigned char *data = command.GetBuffer(SB_PACKET_UPLOAD_HEADER_SIZE);

		// fill in the missing header values
		MAKE_PACKETPTR_BUF(cpack, data);
		cpack->socket = m_socket.GetSocket();
		cpack->size = size;
		cpack->command = SB_COMMAND_DB_DATA;
		cpack->u.db.tableCmd = GetCommand(DatabaseAccess);
		cpack->u.db.u.upload.operation = SB_DBOP_SET_RECORD;
		cpack->u.db.u.upload.databaseId = dbId;
		cpack->u.db.u.upload.unknown = 0;	// FIXME - what does this mean? observed 0 and 5 here

		command.ReleaseBuffer(size);

		// write
		if( !m_socket.Packet(command, response) ) {
			eout("Database ID: " << dbId);
			eeout(command, response);
			throw BError(m_socket.GetLastStatus(),
				"Controller: error writing to device database");
		}
		else {
			// successful packet transfer, so check the network return code
			MAKE_PACKET(rpack, response);
			if( rpack->command != SB_COMMAND_DB_DONE || rpack->u.db.u.return_code != 0 ) {
				std::ostringstream oss;
				oss << "Controller: device responded with error code (command: "
				    << (unsigned int)rpack->command << ", code: "
				    << (unsigned int)rpack->u.db.u.return_code << ")";
				throw BError(oss.str());
			}
		}
	}
}


} // namespace Barry

