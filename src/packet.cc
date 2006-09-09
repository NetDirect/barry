///
/// \file	packet.cc
///		Low level protocol packet builder class.
///		Has knowledge of specific protocol commands in order
///		to hide protocol details behind an API.
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

#include "packet.h"
#include "controller.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "endian.h"
#include "parser.h"
#include "builder.h"
#include "error.h"
/*
#include "common.h"
*/

#define __DEBUG_MODE__
#include "debug.h"


namespace Barry {

Packet::Packet(Controller &con, Data &send, Data &receive)
	: m_con(con)
	, m_send(send)
	, m_receive(receive)
	, m_last_dbop(0)
{
}

Packet::~Packet()
{
}

//
// ClearDatabase
//
/// Builds a command packet for the CLEAR_DATABASE command code, placing
/// the data in the send buffer.
///
void Packet::ClearDatabase(unsigned int dbId)
{
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(9));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(9);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_CLEAR_DATABASE;
	packet.u.db.u.command.databaseId = htobs(dbId);

	m_send.ReleaseBuffer(9);

	m_last_dbop = SB_DBOP_CLEAR_DATABASE;
}

//
// GetDBDB
//
/// Builds a command packet for the GET_DBDB command code, placing the
/// data in m_send.
///
void Packet::GetDBDB()
{
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(7));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(7);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
//	packet.u.db.u.command.operation = SB_DBOP_GET_DBDB;
	packet.u.db.u.command.operation = SB_DBOP_OLD_GET_DBDB;

	m_send.ReleaseBuffer(7);

	m_last_dbop = SB_DBOP_OLD_GET_DBDB;
}

//
// GetRecordStateTable
//
/// Builds a command packet in the send buffer for the
/// GET_RECORD_STATE_TABLE command.
///
void Packet::GetRecordStateTable(unsigned int dbId)
{
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(9));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(9);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_GET_RECORD_STATE_TABLE;
	packet.u.db.u.command.databaseId = htobs(dbId);

	m_send.ReleaseBuffer(9);

	m_last_dbop = SB_DBOP_GET_RECORD_STATE_TABLE;
}

//
// SetRecordFlags
//
/// Builds a command packet in the send buffer for the SET_RECORD_FLAGS
/// command code.
///
/// FIXME - this API call is incomplete, since there are unknown flags
///         in the SetRecordFlags protocol packet.  Currently it is only
///         used to set all flags to zero.
///
void Packet::SetRecordFlags(unsigned int dbId, unsigned int stateTableIndex,
			    uint8_t flag1)
{
	size_t size = SB_PACKET_COMMAND_HEADER_SIZE + DBC_RECORD_FLAGS_SIZE;
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(size));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(size);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_SET_RECORD_FLAGS;
	packet.u.db.u.command.databaseId = htobs(dbId);
	packet.u.db.u.command.u.flags.unknown = flag1;
	packet.u.db.u.command.u.flags.index = htobs(stateTableIndex);
	memset(packet.u.db.u.command.u.flags.unknown2, 0, sizeof(packet.u.db.u.command.u.flags.unknown2));

	m_send.ReleaseBuffer(size);

	m_last_dbop = SB_DBOP_SET_RECORD_FLAGS;
}

//
// DeleteRecordByIndex
//
/// Builds a command packet in the send buffer for the DELETE_RECORD_BY_INDEX
/// command code.
///
void Packet::DeleteRecordByIndex(unsigned int dbId, unsigned int stateTableIndex)
{
	size_t size = SB_PACKET_COMMAND_HEADER_SIZE + DBC_RECORD_HEADER_SIZE;
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(size));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(size);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_DELETE_RECORD_BY_INDEX;
	packet.u.db.u.command.databaseId = htobs(dbId);
	packet.u.db.u.command.u.record.recordIndex = htobs(stateTableIndex);

	m_send.ReleaseBuffer(size);

	m_last_dbop = SB_DBOP_DELETE_RECORD_BY_INDEX;
}

//
// GetRecordByIndex
//
/// Builds a command packet in the send buffer for the GET_RECORD_BY_INDEX
/// command code.
///
void Packet::GetRecordByIndex(unsigned int dbId, unsigned int stateTableIndex)
{
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(11));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(11);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_GET_RECORD_BY_INDEX;
	packet.u.db.u.command.databaseId = htobs(dbId);
	packet.u.db.u.command.u.record.recordIndex = htobs(stateTableIndex);

	m_send.ReleaseBuffer(11);

	m_last_dbop = SB_DBOP_GET_RECORD_BY_INDEX;
}

//
// SetRecordByIndex
//
/// Builds a command packet in the m_send buffer for the SET_RECORD_BY_INDEX
/// command code.
///
/// \return	bool
///		- true means success
///		- false means no data available from Builder object
///
bool Packet::SetRecordByIndex(unsigned int dbId, unsigned int stateTableIndex,
			      Builder &build)
{
	// get new data if available
	if( !build.Retrieve(dbId) )
		return false;

	// build packet data
	size_t header_size = SB_PACKET_COMMAND_HEADER_SIZE + DBC_INDEXED_UPLOAD_HEADER_SIZE;
	build.BuildFields(m_send, header_size);
	size_t total_size = m_send.GetSize();

	// fill in the header values
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(total_size));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(total_size);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_SET_RECORD_BY_INDEX;
	packet.u.db.u.command.databaseId = htobs(dbId);
	packet.u.db.u.command.u.index_upload.unknown = 0;
	packet.u.db.u.command.u.index_upload.index = htobs(stateTableIndex);

	m_send.ReleaseBuffer(total_size);

	m_last_dbop = SB_DBOP_SET_RECORD_BY_INDEX;
	return true;
}

//
// GetRecords
//
/// Builds a command packet in the send buffer for the GET_RECORDS
/// command code.
///
void Packet::GetRecords(unsigned int dbId)
{
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(9));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(9);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_OLD_GET_RECORDS;
	packet.u.db.u.command.databaseId = htobs(dbId);

	m_send.ReleaseBuffer(9);

	m_last_dbop = SB_DBOP_OLD_GET_RECORDS;
}

//
// SetRecord
//
/// Builds a command packet in the m_send buffer for the SET_RECORD command
/// code.
///
/// \return	bool
///		- true means success
///		- false means no data available from Builder object
///
bool Packet::SetRecord(unsigned int dbId, Builder &build)
{
	// get new data if available
	if( !build.Retrieve(dbId) )
		return false;

	// build packet data
	size_t header_size = SB_PACKET_COMMAND_HEADER_SIZE + DBC_TAGGED_UPLOAD_HEADER_SIZE;
	build.BuildHeader(m_send, header_size);
	build.BuildFields(m_send, header_size);
	size_t total_size = m_send.GetSize();

	// fill in the header values
	MAKE_PACKETPTR_BUF(cpack, m_send.GetBuffer(total_size));
	Protocol::Packet &packet = *cpack;

	packet.socket = htobs(m_con.m_socket.GetSocket());
	packet.size = htobs(total_size);
	packet.command = SB_COMMAND_DB_DATA;
	packet.u.db.tableCmd = m_con.GetCommand(Controller::DatabaseAccess);
	packet.u.db.u.command.operation = SB_DBOP_SET_RECORD;
	packet.u.db.u.command.databaseId = htobs(dbId);
	packet.u.db.u.command.u.tag_upload.unknown = 0;
	packet.u.db.u.command.u.tag_upload.uniqueId = htobl(build.GetUniqueId());
	packet.u.db.u.command.u.tag_upload.unknown2 = 1;	// unknown observed value

	m_send.ReleaseBuffer(total_size);

	m_last_dbop = SB_DBOP_SET_RECORD;
	return true;
}


//
// Command
//
/// Returns the command value of the receive packet.  If receive isn't
/// large enough, throws BError.
///
unsigned int Packet::Command() const
{
	Protocol::CheckSize(m_receive);
	MAKE_PACKET(rpack, m_receive);
	return rpack->command;
}

// throws FIXME if packet doesn't support it
unsigned int Packet::ReturnCode() const
{
	if( Command() == SB_COMMAND_DB_DONE ) {
		Protocol::CheckSize(SB_PACKET_DBACCESS_HEADER_SIZE + SB_DBACCESS_RETURN_CODE_SIZE);
		MAKE_PACKET(rpack, m_receive);
		return rpack->u.db.u.return_code;
	}
	else {
		throw BError("Attempting to extract a return code from the wrong response packet type");
	}
}

//
// DBOperation
//
/// Returns the database operation code from the receive packet, assuming
/// that receive contains a response packet.  If receive isn't large
/// enough, throws BError.
///
unsigned int Packet::DBOperation() const
{
	Protocol::CheckSize(m_receive, SB_PACKET_RESPONSE_HEADER_SIZE);
	MAKE_PACKET(rpack, m_receive);
	return rpack->u.db.u.response.operation;
}

//
// Parse
//
/// Parses the data in the receive buffer, and attempts to be smart about it,
/// using the last send command as guidance for what to expect in the
/// response.
///
/// \returns	bool	true - packet was recognized and parse was attempted
///			false - packet was not recognized
///
bool Packet::Parse(Parser &parser)
{
	size_t offset = 0;
	MAKE_PACKET(rpack, m_receive);

	switch( m_last_dbop )
	{
	case SB_DBOP_OLD_GET_RECORDS:
	case SB_DBOP_GET_RECORD_BY_INDEX:
		parser.Clear();

		offset = SB_PACKET_RESPONSE_HEADER_SIZE + DBR_OLD_TAGGED_RECORD_HEADER_SIZE;
		Protocol::CheckSize(m_receive, offset);
		// FIXME - this may need adjustment for email records... they
		// don't seem to have uniqueID's
		parser.SetUniqueId(btohl(rpack->u.db.u.response.u.tagged.uniqueId));

		parser.ParseHeader(m_receive, offset);
		parser.ParseFields(m_receive, offset);
		parser.Store();
		return true;

	default:	// unknown command
		return false;
	}
}

} // namespace Barry

