///
/// \file	controller.cc
///		High level Barry API class
///

/*
    Copyright (C) 2005-2006, Net Direct Inc. (http://www.netdirect.ca/)

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
	m_pin(device.m_pin),
	m_socket(m_dev, device.m_ep.write, device.m_ep.read),
	m_mode(Unspecified)
{
	if( !m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
		throw BError(m_dev.GetLastError(),
			"Controller: SetConfiguration failed");

	m_iface = new Usb::Interface(m_dev, BLACKBERRY_INTERFACE);
}

Controller::~Controller()
{
	delete m_iface;
}

///////////////////////////////////////////////////////////////////////////////
// protected members

void Controller::SelectMode(ModeType mode, uint16_t &socket, uint8_t &flag)
{
	// select mode
	Packet packet;
	packet.socket = 0;
	packet.size = htobs(SB_MODE_PACKET_COMMAND_SIZE);
	packet.command = SB_COMMAND_SELECT_MODE;
	packet.u.mode.socket = htobs(SB_MODE_REQUEST_SOCKET);
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
	Data command(&packet, btohs(packet.size));
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
	socket = btohs(modepack->u.mode.socket);
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
		if( rpack->command == SB_COMMAND_DB_DATA && btohs(rpack->size) > 10 ) {
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
	packet.socket = htobs(m_socket.GetSocket());
	packet.size = htobs(7);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
//	packet.u.db.u.command.operation = SB_DBOP_GET_DBDB;
	packet.u.db.u.command.operation = SB_DBOP_OLD_GET_DBDB;

	Data command(&packet, btohs(packet.size));
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
	unsigned int ID = 0;
	// FIXME - this needs a better error handler...
	if( !m_dbdb.GetDBNumber(name, ID) ) {
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

	// start fresh
	result.Clear();

	Packet packet;
	packet.socket = htobs(m_socket.GetSocket());
	packet.size = htobs(9);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_GET_RECORD_STATE_TABLE;
	packet.u.db.u.command.databaseId = htobs(dbId);

	Data command(&packet, btohs(packet.size));
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eout("Database ID: " << dbId);
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error loading database");
	}

	result.Parse(response);

	// flush the command sequence
	MAKE_PACKET(rpack, response);
	while( response.GetSize() >= SB_PACKET_HEADER_SIZE &&
	       rpack->command != SB_COMMAND_DB_DONE )
	{
		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw BError(m_socket.GetLastStatus(),
				"Controller: error loading state table (next)");
		}
		rpack = (const Packet *) response.GetData();
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

	Packet packet;
	packet.socket = htobs(m_socket.GetSocket());
	packet.size = htobs(11);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
	packet.u.db.u.record_cmd.operation = SB_DBOP_GET_RECORD_BY_INDEX;
	packet.u.db.u.record_cmd.databaseId = htobs(dbId);
	packet.u.db.u.record_cmd.recordIndex = htobs(stateTableIndex);

	Data command(&packet, btohs(packet.size));
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eout("Database ID: " << dbId);
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error loading database");
	}

	MAKE_PACKET(rpack, response);

	// perform copious packet checks
	if( response.GetSize() < SB_PACKET_HEADER_SIZE ||
	    response.GetSize() < SB_PACKET_OLD_RESPONSE_HEADER_SIZE ) {
		eeout(command, response);

		std::ostringstream oss;
		oss << "Controller: invalid response packet size of "
		    << response.GetSize();
		eout(oss.str());
		throw BError(oss.str());
	}
	if( rpack->command != SB_COMMAND_DB_DATA ) {
		eeout(command, response);

		std::ostringstream oss;
		oss << "Controller: unexpected command of 0x"
		    << std::setbase(16) << (unsigned int)rpack->command
		    << " instead of expected 0x"
		    << std::setbase(16) << (unsigned int)SB_COMMAND_DB_DATA;
		eout(oss.str());
		throw BError(oss.str());
	}

	// grab that data
	parser(response, SB_PACKET_OLD_RESPONSE_HEADER_SIZE);

	// flush the command sequence
	while( response.GetSize() >= SB_PACKET_HEADER_SIZE &&
	       rpack->command != SB_COMMAND_DB_DONE )
	{
		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw BError(m_socket.GetLastStatus(),
				"Controller: error loading state table (next)");
		}
		rpack = (const Packet *) response.GetData();
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

	size_t size = SB_PACKET_DBACCESS_HEADER_SIZE + DB_RECORD_FLAGS_COMMAND_SIZE;

	Packet packet;
	packet.socket = htobs(m_socket.GetSocket());
	packet.size = htobs(size);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
	packet.u.db.u.rf_cmd.operation = SB_DBOP_SET_RECORD_FLAGS;
	packet.u.db.u.rf_cmd.databaseId = htobs(dbId);
	packet.u.db.u.rf_cmd.flags.unknown = 0;
	packet.u.db.u.rf_cmd.flags.index = htobs(stateTableIndex);
	memset(packet.u.db.u.rf_cmd.flags.unknown2, 0, sizeof(packet.u.db.u.rf_cmd.flags.unknown2));

	Data command(&packet, size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eout("Database ID: " << dbId);
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error loading database");
	}

	MAKE_PACKET(rpack, response);

	// flush the command sequence
	while( response.GetSize() >= SB_PACKET_HEADER_SIZE &&
	       rpack->command != SB_COMMAND_DB_DONE )
	{
		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw BError(m_socket.GetLastStatus(),
				"Controller: error loading state table (next)");
		}
		rpack = (const Packet *) response.GetData();
	}
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

	size_t size = SB_PACKET_DBACCESS_HEADER_SIZE + DB_RECORD_COMMAND_HEADER_SIZE;

	Packet packet;
	packet.socket = htobs(m_socket.GetSocket());
	packet.size = htobs(size);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
	packet.u.db.u.record_cmd.operation = SB_DBOP_DELETE_RECORD_BY_INDEX;
	packet.u.db.u.record_cmd.databaseId = htobs(dbId);
	packet.u.db.u.record_cmd.recordIndex = htobs(stateTableIndex);

	Data command(&packet, size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eout("Database ID: " << dbId);
		eeout(command, response);
		throw BError(m_socket.GetLastStatus(),
			"Controller: error deleting record");
	}

	MAKE_PACKET(rpack, response);

	// flush the command sequence
	while( response.GetSize() >= SB_PACKET_HEADER_SIZE &&
	       rpack->command != SB_COMMAND_DB_DONE )
	{
		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw BError(m_socket.GetLastStatus(),
				"Controller: error deleting record (next)");
		}
		rpack = (const Packet *) response.GetData();
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
	packet.socket = htobs(m_socket.GetSocket());
	packet.size = htobs(9);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
//	packet.u.db.u.command.operation = SB_DBOP_GET_RECORDS;
	packet.u.db.u.command.operation = SB_DBOP_OLD_GET_RECORDS;
	packet.u.db.u.command.databaseId = htobs(dbId);

	Data command(&packet, btohs(packet.size));
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

	// Protocol note: so far in testing, this CLEAR_DATABASE operation is
	//                required, since every record sent via SET_RECORD
	//                is treated like a hypothetical "ADD_RECORD" (perhaps
	//                SET_RECORD should be renamed)... I don't know if
	//                there is a real SET_RECORD... all I know is from
	//                the Windows USB captures, which uses this same
	//                technique.
	Packet packet;
	packet.socket = htobs(m_socket.GetSocket());
	packet.size = htobs(9);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = GetCommand(DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_CLEAR_DATABASE;
	packet.u.db.u.command.databaseId = htobs(dbId);

	Data command(&packet, btohs(packet.size));
	Data response;

	// FIXME - sometimes this takes a long time... find out if there
	// is a timeout mechanism or something needed as well
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
		cpack->socket = htobs(m_socket.GetSocket());
		cpack->size = htobs(size);
		cpack->command = SB_COMMAND_DB_DATA;
		cpack->u.db.tableCmd = GetCommand(DatabaseAccess);
		cpack->u.db.u.upload.operation = SB_DBOP_SET_RECORD;
		cpack->u.db.u.upload.databaseId = htobs(dbId);
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

