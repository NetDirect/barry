///
/// \file	m_desktop.cc
///		Mode class for the Desktop mode
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "m_desktop.h"
#include "data.h"
#include "protocol.h"
#include "protostructs.h"
#include "packet.h"
#include "endian.h"
#include "error.h"
#include "usbwrap.h"
#include "controller.h"
#include <stdexcept>
#include <sstream>

#include "debug.h"

namespace Barry { namespace Mode {


///////////////////////////////////////////////////////////////////////////////
// Desktop Mode class

Desktop::Desktop(Controller &con)
	: m_con(con)
	, m_ModeSocket(0)
	, m_ic(0)
{
}

Desktop::Desktop(Controller &con, const IConverter &ic)
	: m_con(con)
	, m_ModeSocket(0)
	, m_ic(&ic)
{
}

Desktop::~Desktop()
{
}

///////////////////////////////////////////////////////////////////////////////
// protected members

void Desktop::LoadCommandTable()
{
	char rawCommand[] = { 6, 0, 0x0a, 0, 0x40, 0, 0, 1, 0, 0 };
	*((uint16_t*) rawCommand) = htobs(m_socket->GetSocket());

	Data command(rawCommand, sizeof(rawCommand));
	Data response;

	try {
		m_socket->Packet(command, response);

		MAKE_PACKET(rpack, response);
		while( rpack->command != SB_COMMAND_DB_DONE ) {
			m_socket->NextRecord(response);

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
		eout("Desktop: error getting command table");
		eeout(command, response);
		throw;
	}
}

void Desktop::LoadDBDB()
{
	Data command, response;
	DBPacket packet(*this, command, response);
	packet.GetDBDB();

	m_socket->Packet(packet);

	while( packet.Command() != SB_COMMAND_DB_DONE ) {
		if( packet.Command() == SB_COMMAND_DB_DATA ) {
			m_dbdb.Clear();
			m_dbdb.Parse(response);
		}

		// advance!
		m_socket->NextRecord(response);
	}
}



///////////////////////////////////////////////////////////////////////////////
// public API

//
// Open
//
/// Select device mode.  This is required before using any other mode-based
/// operations, such as GetDBDB() and LoadDatabase().
///
/// This function opens a socket to the device for communicating in Desktop
/// mode.  If the device requires it, specify the password with a const char*
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
void Desktop::Open(const char *password)
{
	if( m_ModeSocket ) {
		m_socket->Close();
		m_socket.reset();
		m_ModeSocket = 0;
	}

	m_ModeSocket = m_con.SelectMode(Controller::Desktop);
	RetryPassword(password);
}

//
// RetryPassword
//
/// Retry a failed password attempt from the first call to Open().
/// Only call this function in response to Barry::BadPassword exceptions
/// that are thrown from Open().
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
void Desktop::RetryPassword(const char *password)
{
	if( m_socket.get() != 0 )
		throw std::logic_error("Socket alreay open in RetryPassword");

	m_socket = m_con.m_zero.Open(m_ModeSocket, password);

	// get command table and database database
	LoadCommandTable();
	LoadDBDB();
}

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
unsigned int Desktop::GetDBID(const std::string &name) const
{
	unsigned int ID = 0;
	// FIXME - this needs a better error handler...
	if( !m_dbdb.GetDBNumber(name, ID) ) {
		throw Error("Desktop: database name not found: " + name);
	}
	return ID;
}

//
// GetDBCommand
//
/// Get database command from command table.  Must call Open()
/// before this.
///
unsigned int Desktop::GetDBCommand(CommandType ct)
{
	unsigned int cmd = 0;
	const char *cmdName = "Unknown";

	switch( ct )
	{
	case DatabaseAccess:
		cmdName = "Database Access";
		cmd = m_commandTable.GetCommand(cmdName);
		break;
	default:
		throw std::logic_error("Desktop: unknown command type");
	}

	if( cmd == 0 ) {
		std::ostringstream oss;
		oss << "Desktop: unable to get command code: " << cmdName;
		throw Error(oss.str());
	}

	return cmd;
}

void Desktop::SetIConverter(const IConverter &ic)
{
	m_ic = &ic;
}

//
// GetRecordStateTable
//
/// Retrieve the record state table from the handheld device, using the given
/// database ID.  Results will be stored in result, which will be cleared
/// before adding.
///
void Desktop::GetRecordStateTable(unsigned int dbId, RecordStateTable &result)
{
	dout("Database ID: " << dbId);

	// start fresh
	result.Clear();

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.GetRecordStateTable(dbId);

	m_socket->Packet(packet);
	result.Parse(response);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket->NextRecord(response);
}

//
// AddRecord
//
/// Adds a record to the specified database.  RecordId is
/// retrieved from build, and duplicate IDs are allowed by the device
/// (i.e. you can have two records with the same ID) 
/// but *not* recommended!
//
void Desktop::AddRecord(unsigned int dbId, Builder &build)
{
	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);

	if( packet.SetRecord(dbId, build, m_ic) ) {

		std::ostringstream oss;

		m_socket->Packet(packet);

		// successful packet transfer, so check the network return code
		if( packet.Command() != SB_COMMAND_DB_DONE ) {
			oss << "Desktop: device responded with unexpected packet command code: "
			    << "0x" << std::hex << packet.Command();
			throw Error(oss.str());
		}

		if( packet.ReturnCode() != 0 ) {
			oss << "Desktop: device responded with error code (command: "
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
void Desktop::GetRecord(unsigned int dbId,
			   unsigned int stateTableIndex,
			   Parser &parser)
{
	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.GetRecordByIndex(dbId, stateTableIndex);

	m_socket->Packet(packet);

	// perform copious packet checks
	if( response.GetSize() < SB_PACKET_RESPONSE_HEADER_SIZE ) {
		eeout(command, response);

		std::ostringstream oss;
		oss << "Desktop: invalid response packet size of "
		    << std::dec << response.GetSize();
		eout(oss.str());
		throw Error(oss.str());
	}
	if( packet.Command() != SB_COMMAND_DB_DATA ) {
		eeout(command, response);

		std::ostringstream oss;
		oss << "Desktop: unexpected command of 0x"
		    << std::setbase(16) << packet.Command()
		    << " instead of expected 0x"
		    << std::setbase(16) << (unsigned int)SB_COMMAND_DB_DATA;
		eout(oss.str());
		throw Error(oss.str());
	}

	// grab that data
	packet.Parse(parser, m_ic);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket->NextRecord(response);
}

//
// SetRecord
//
/// Overwrites a specific record in the device as identified by the
/// stateTableIndex.
///
void Desktop::SetRecord(unsigned int dbId, unsigned int stateTableIndex,
			   Builder &build)
{
	dout("Database ID: " << dbId << " Index: " << stateTableIndex);

	Data command, response;
	DBPacket packet(*this, command, response);

	// loop until builder object has no more data
	if( !packet.SetRecordByIndex(dbId, stateTableIndex, build, m_ic) ) {
		throw std::logic_error("Desktop: no data available in SetRecord");
	}

	m_socket->Packet(packet);

	std::ostringstream oss;

	// successful packet transfer, so check the network return code
	if( packet.Command() != SB_COMMAND_DB_DONE ) {
		oss << "Desktop: device responded with unexpected packet command code: "
		    << "0x" << std::hex << packet.Command();
		throw Error(oss.str());
	}

	if( packet.ReturnCode() != 0 ) {
		oss << "Desktop: device responded with error code (command: "
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
void Desktop::ClearDirty(unsigned int dbId, unsigned int stateTableIndex)
{
	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.SetRecordFlags(dbId, stateTableIndex, 0);

	m_socket->Packet(packet);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket->NextRecord(response);
}

//
// DeleteRecord
//
/// Deletes the specified record in the specified database.
///
void Desktop::DeleteRecord(unsigned int dbId, unsigned int stateTableIndex)
{
	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.DeleteRecordByIndex(dbId, stateTableIndex);

	m_socket->Packet(packet);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket->NextRecord(response);
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
void Desktop::LoadDatabase(unsigned int dbId, Parser &parser)
{
	dout("Database ID: " << dbId);

	Data command, response;
	DBPacket packet(*this, command, response);
	packet.GetRecords(dbId);

	m_socket->Packet(packet);

	while( packet.Command() != SB_COMMAND_DB_DONE ) {
		if( packet.Command() == SB_COMMAND_DB_DATA ) {
			// this size is the old header size, since using
			// old command above
			packet.Parse(parser, m_ic);
		}

		// advance!
		m_socket->NextRecord(response);
	}
}

void Desktop::SaveDatabase(unsigned int dbId, Builder &builder)
{
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
	m_socket->Packet(packet, 60000);
	if( packet.ReturnCode() != 0 ) {
		std::ostringstream oss;
		oss << "Desktop: could not clear database: (command: "
		    << "0x" << std::hex << packet.Command() << ", code: "
		    << "0x" << std::hex << packet.ReturnCode() << ")";
		throw Error(oss.str());
	}

	// check response to clear command was successful
	if( packet.Command() != SB_COMMAND_DB_DONE ) {
		eeout(command, response);
		throw Error("Desktop: error clearing database, bad response");
	}

	// loop until builder object has no more data
	bool first = true;
	while( packet.SetRecord(dbId, builder, m_ic) ) {
		dout("Database ID: " << dbId);

		m_socket->Packet(packet, first ? 60000 : -1);
		first = false;

		std::ostringstream oss;
		// successful packet transfer, so check the network return code
		if( packet.Command() != SB_COMMAND_DB_DONE ) {
			oss << "Desktop: device responded with unexpected packet command code: "
			    << "0x" << std::hex << packet.Command();
			throw Error(oss.str());
		}

		if( packet.ReturnCode() != 0 ) {
			oss << "Desktop: device responded with error code (command: "
			    << packet.Command() << ", code: "
			    << packet.ReturnCode() << ")";
			throw Error(oss.str());
		}
	}
}

}} // namespace Barry::Mode

