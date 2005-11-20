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
#include "error.h"
#include "data.h"
#include "parser.h"

#define __DEBUG_MODE__
#include "debug.h"

#include <sstream>

#include <iomanip>

namespace Barry {

Controller::Controller(const ProbeResult &device)
	: m_dev(device.m_dev),
	m_iface(m_dev, BLACKBERRY_INTERFACE),
	m_pin(device.m_pin),
	m_socket(m_dev, WRITE_ENDPOINT, READ_ENDPOINT),
	m_mode(Unspecified)
{
	if( !m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
		throw SBError(m_dev.GetLastError(),
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
	packet.data.mode.socket = SB_MODE_REQUEST_SOCKET;
	packet.data.mode.flag = 0x05;	// FIXME
	memset(packet.data.mode.modeName, 0,
		sizeof(packet.data.mode.modeName));

	char *modeName = (char *) packet.data.mode.modeName;
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
		throw SBError(m_socket.GetLastStatus(),
			"Controller: error setting desktop mode");
	}

	// get the data socket number
	// indicates the socket number that
	// should be used below in the Open() call
	CheckSize(response, SB_MODE_PACKET_RESPONSE_SIZE);
	MAKE_PACKET(modepack, response);
	if( modepack->command != SB_COMMAND_MODE_SELECTED ) {
		eeout(command, response);
		throw SBError("Controller: mode not selected");
	}

	// return the socket and flag that the device is expecting us to use
	socket = modepack->data.mode.socket;
	flag = modepack->data.mode.flag + 1;
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
		throw SBError(oss.str());
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
		throw SBError(m_socket.GetLastStatus(),
			"Controller: error getting command table");
	}

	MAKE_PACKET(firstpack, response);
	while( firstpack->command != SB_COMMAND_DB_DONE ) {
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw SBError(m_socket.GetLastStatus(),
				"Controller: error getting command table(next)");
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

void Controller::LoadDBDB()
{
	assert( m_mode == Desktop );

	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 7;
	packet.command = SB_COMMAND_DB_DATA;
	packet.data.db.tableCmd = GetCommand(DatabaseAccess);
//	packet.data.db.data.db.operation = SB_DBOP_GET_DBDB;
	packet.data.db.data.db.operation = SB_DBOP_OLD_GET_DBDB;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
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
			throw SBError(m_socket.GetLastStatus(),
				"Controller: error getting command table(next)");
		}
		rpack = (const Packet *) response.GetData();
	}

	ddout(m_dbdb);
}


///////////////////////////////////////////////////////////////////////////////
// public API

unsigned int Controller::GetDBID(const std::string &name) const
{
	unsigned int ID = m_dbdb.GetDBNumber(name);
	// FIXME - this needs a better error handler... the dbdb needs one too!
	if( ID == 0 ) {
		throw SBError("Controller: Address Book not found");
	}
	return ID;
}

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

void Controller::LoadDatabase(unsigned int dbId, Parser &parser)
{
	if( m_mode != Desktop )
		throw std::logic_error("Wrong mode in LoadDatabase");

	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 9;
	packet.command = SB_COMMAND_DB_DATA;
	packet.data.db.tableCmd = GetCommand(DatabaseAccess);
//	packet.data.db.data.db.operation = SB_DBOP_GET_RECORDS;
	packet.data.db.data.db.operation = SB_DBOP_OLD_GET_RECORDS;
	packet.data.db.data.db.databaseId = dbId;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eout("Database ID: " << dbId);
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Controller: error loading database");
	}

	MAKE_PACKET(rpack, response);
	while( rpack->command != SB_COMMAND_DB_DONE ) {
		if( rpack->command == SB_COMMAND_DB_DATA ) {
			parser(response);
		}

		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw SBError(m_socket.GetLastStatus(),
				"Controller: error loading database (next)");
		}
		rpack = (const Packet *) response.GetData();
	}
}


} // namespace Barry

