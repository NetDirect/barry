///
/// \file	m_desktop.cc
///		Mode class for the Desktop mode
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "parser.h"
#include <stdexcept>
#include <sstream>

#include "debug.h"

namespace Barry { namespace Mode {


///////////////////////////////////////////////////////////////////////////////
// Desktop Mode class

Desktop::Desktop(Controller &con)
	: Mode(con, Controller::Desktop)
	, m_ic(0)
{
}

Desktop::Desktop(Controller &con, const IConverter &ic)
	: Mode(con, Controller::Desktop)
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

	try {
		m_socket->Packet(command, m_response);

		MAKE_PACKET(rpack, m_response);
		while( rpack->command != SB_COMMAND_DB_DONE ) {
			m_socket->NextRecord(m_response);

			rpack = (const Protocol::Packet *) m_response.GetData();
			if( rpack->command == SB_COMMAND_DB_DATA && btohs(rpack->size) > 10 ) {
				// second packet is generally large, and contains
				// the command table
				m_commandTable.Clear();
				m_commandTable.Parse(m_response, 6);
			}
		}

		ddout(m_commandTable);

	}
	catch( Usb::Error & ) {
		eout("Desktop: error getting command table");
		eeout(command, m_response);
		throw;
	}
}

void Desktop::LoadDBDB()
{
	DBPacket packet(*this, m_command, m_response);
	packet.GetDBDB();

	m_socket->Packet(packet);

	while( packet.Command() != SB_COMMAND_DB_DONE ) {
		if( packet.Command() == SB_COMMAND_DB_DATA ) {
			m_dbdb.Clear();
			m_dbdb.Parse(m_response);
		}

		// advance!
		m_socket->NextRecord(m_response);
	}
}

void Desktop::OnOpen()
{
	// get command table and database database
	LoadCommandTable();
	LoadDBDB();
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

	DBPacket packet(*this, m_command, m_response);
	packet.GetRecordStateTable(dbId);

	m_socket->Packet(packet);
	result.Parse(m_response);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket->NextRecord(m_response);
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

	DBPacket packet(*this, m_command, m_response);

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

	std::string dbName;
	m_dbdb.GetDBName(dbId, dbName);

	DBPacket packet(*this, m_command, m_response);
	packet.GetRecordByIndex(dbId, stateTableIndex);

	m_socket->Packet(packet);

	// perform copious packet checks
	if( m_response.GetSize() < SB_PACKET_RESPONSE_HEADER_SIZE ) {
		eeout(m_command, m_response);

		std::ostringstream oss;
		oss << "Desktop: invalid response packet size of "
		    << std::dec << m_response.GetSize();
		eout(oss.str());
		throw Error(oss.str());
	}
	if( packet.Command() != SB_COMMAND_DB_DATA ) {
		eeout(m_command, m_response);

		std::ostringstream oss;
		oss << "Desktop: unexpected command of 0x"
		    << std::setbase(16) << packet.Command()
		    << " instead of expected 0x"
		    << std::setbase(16) << (unsigned int)SB_COMMAND_DB_DATA;
		eout(oss.str());
		throw Error(oss.str());
	}

	// grab that data
	packet.Parse(parser, dbName, m_ic);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket->NextRecord(m_response);
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

	DBPacket packet(*this, m_command, m_response);

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

	DBPacket packet(*this, m_command, m_response);
	packet.SetRecordFlags(dbId, stateTableIndex, 0);

	m_socket->Packet(packet);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket->NextRecord(m_response);
}

//
// DeleteRecord
//
/// Deletes the specified record in the specified database.
///
void Desktop::DeleteRecord(unsigned int dbId, unsigned int stateTableIndex)
{
	dout("Database ID: " << dbId);

	DBPacket packet(*this, m_command, m_response);
	packet.DeleteRecordByIndex(dbId, stateTableIndex);

	m_socket->Packet(packet);

	// flush the command sequence
	while( packet.Command() != SB_COMMAND_DB_DONE )
		m_socket->NextRecord(m_response);
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
	DBData data;
	DBLoader loader(*this, data);
	bool loading = loader.StartDBLoad(dbId);
	while( loading ) {
		// manual parser call
		parser.ParseRecord(data, m_ic);

		// advance!
		loading = loader.GetNextRecord();
	}
}

void Desktop::ClearDatabase(unsigned int dbId)
{
	dout("Database ID: " << dbId);

	DBPacket packet(*this, m_command, m_response);
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
		eeout(m_command, m_response);
		throw Error("Desktop: error clearing database, bad response");
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
	ClearDatabase(dbId);

	DBPacket packet(*this, m_command, m_response);

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



//////////////////////////////////////////////////////////////////////////////
// DBLoader class


struct DBLoaderData
{
	DBPacket m_packet;
	DBLoaderData(Desktop &desktop, Data &command, Data &response)
		: m_packet(desktop, command, response)
	{
	}
};

DBLoader::DBLoader(Desktop &desktop, DBData &data)
	: m_loader(new DBLoaderData(desktop, desktop.m_command, data.UseData()))
	, m_desktop(desktop)
	, m_data(data)
	, m_loading(false)
{
}

DBLoader::~DBLoader()
{
	delete m_loader;
}

bool DBLoader::StartDBLoad(unsigned int dbId)
{
	dout("Database ID: " << dbId);

	m_loading = true;
	m_desktop.m_dbdb.GetDBName(dbId, m_dbName);

	m_loader->m_packet.GetRecords(dbId);
	m_desktop.m_socket->Packet(m_loader->m_packet);

	while( m_loader->m_packet.Command() != SB_COMMAND_DB_DONE ) {
		if( m_loader->m_packet.Command() == SB_COMMAND_DB_DATA ) {
			// this size is the old header size, since using
			// old command above
			m_loader->m_packet.ParseMeta(m_data);
			m_data.SetDBName(m_dbName);
			return true;
		}

		// advance!
		m_desktop.m_socket->NextRecord(m_data.UseData());
	}

	m_loading = false;
	return false;
}

bool DBLoader::GetNextRecord()
{
	if( !m_loading )
		return false;

	do {
		// advance!
		m_desktop.m_socket->NextRecord(m_data.UseData());

		if( m_loader->m_packet.Command() == SB_COMMAND_DB_DATA ) {
			// this size is the old header size, since using
			// old command above
			m_loader->m_packet.ParseMeta(m_data);
			return true;
		}
	} while( m_loader->m_packet.Command() != SB_COMMAND_DB_DONE );

	m_loading = false;
	return false;
}

}} // namespace Barry::Mode

