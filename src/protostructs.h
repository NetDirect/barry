///
/// \file	protostructs.h
///		USB Blackberry bulk protocol API.  This is split out from
///		protocol.h so that low level, packed structs can be
///		compiled separately from the application.  This prevents
///		aliasing problems in the application, or using
///		-fno-strict-aliasing, which the library only needs.
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

#ifndef __BARRY_PROTOSTRUCTS_H__
#define __BARRY_PROTOSTRUCTS_H__

#include <stdint.h>
#include <sys/types.h>

// forward declarations
class Data;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
union SizePacket
{
	uint16_t size;
	char buffer[4];
} __attribute__ ((packed));

///////////////////////////////////////////////////////////////////////////////
struct SocketCommand
{
	uint16_t	socket;
	uint8_t		param;
} __attribute__ ((packed));

///////////////////////////////////////////////////////////////////////////////
struct SequenceCommand
{
	uint8_t		unknown1;
	uint8_t		unknown2;
	uint8_t		unknown3;
	uint32_t	sequenceId;
} __attribute__ ((packed));

///////////////////////////////////////////////////////////////////////////////
struct ModeSelectCommand
{
	uint16_t	socket;
	uint8_t		flag;
	uint8_t		modeName[16];
	struct ResponseBlock
	{
		uint8_t		unknown[20];
	} __attribute__ ((packed)) response;
} __attribute__ ((packed));

///////////////////////////////////////////////////////////////////////////////
struct DBCommand
{
	uint8_t		operation;	// see below
	uint16_t	databaseId;	// value from the Database Database
	uint8_t		data[1];
} __attribute__ ((packed));
#define DB_COMMAND_HEADER_SIZE		(sizeof(Barry::DBCommand) - 1)

struct DBResponse
{
	uint8_t		operation;
	uint32_t	unknown;
	uint16_t	count;
	uint8_t		data[1];
} __attribute__ ((packed));
#define DB_RESPONSE_HEADER_SIZE		(sizeof(Barry::DBResponse) - 1)

struct OldDBResponse
{
	uint8_t		operation;
	uint8_t		unknown;
	uint16_t	count;
	uint8_t		data[1];
} __attribute__ ((packed));
#define OLD_DB_RESPONSE_HEADER_SIZE	(sizeof(Barry::OldDBResponse) - 1)

struct UploadCommand
{
	uint8_t		operation;
	uint16_t	databaseId;	// value from the Database Database
	uint8_t		unknown;	// observed: 00 or 05
	uint8_t		data[1];
} __attribute__ ((packed));
#define UPLOAD_HEADER_SIZE		(sizeof(Barry::UploadCommand) - 1)



///////////////////////////////////////////////////////////////////////////////
// CommandTable command and field structures

struct CommandTableField
{
	uint8_t		size;		// no null terminator
	uint8_t		code;
	uint8_t		name[1];
} __attribute__ ((packed));

#define COMMAND_FIELD_HEADER_SIZE	(sizeof(Barry::CommandTableField) - 1)



///////////////////////////////////////////////////////////////////////////////
// Database database command and field structures

struct OldDBDBField
{
	uint16_t	dbNumber;
	uint8_t		unknown1;
	uint32_t	dbSize;			// assumed from Cassis docs...
						// always 0 in USB
	uint16_t	dbRecordCount;
	uint16_t	unknown2;
	uint16_t	nameSize;		// includes null terminator
	uint8_t		name[1];
} __attribute__ ((packed));
#define OLD_DBDB_FIELD_HEADER_SIZE	(sizeof(Barry::OldDBDBField) - 1)

struct DBDBField
{
	uint16_t	dbNumber;
	uint8_t		unknown1;
	uint32_t	dbSize;			// assumed from Cassis docs...
						// always 0 in USB
	uint32_t	dbRecordCount;
	uint16_t	unknown2;
	uint16_t	nameSize;		// includes null terminator
	uint8_t		unknown3;
	uint8_t		name[1];		// followed by 2 zeros!
	uint16_t	unknown;		// this comes after the
						// null terminated name, but
						// is here for size calcs
} __attribute__ ((packed));
#define DBDB_FIELD_HEADER_SIZE	(sizeof(Barry::DBDBField) - 1)

struct OldDBDBRecord
{
	uint8_t		operation;
	uint16_t	count;			// number of fields in record
	OldDBDBField	field[1];
} __attribute__ ((packed));
#define OLD_DBDB_RECORD_HEADER_SIZE	(sizeof(Barry::OldDBDBRecord) - sizeof(Barry::OldDBDBField))

struct DBDBRecord
{
	uint8_t		operation;
	uint16_t	count;
	uint8_t		unknown[3];
	DBDBField	field[1];
} __attribute__ ((packed));
#define DBDB_RECORD_HEADER_SIZE		(sizeof(Barry::DBDBRecord) - sizeof(Barry::DBDBField))




///////////////////////////////////////////////////////////////////////////////
// Address book sub-field structs

struct GroupLink
{
	uint32_t	uniqueId;
	uint16_t	unknown;
} __attribute__ ((packed));


///////////////////////////////////////////////////////////////////////////////
// Message sub-field structs

struct MessageAddress
{
	uint8_t		unknown[8];
	uint8_t		addr[1];	// 2 null terminated strings: first
					// contains full name, second contains
					// the email address
} __attribute__ ((packed));





///////////////////////////////////////////////////////////////////////////////
// Common database field structure

struct CommonField
{
	uint16_t	size;		// including null terminator
	uint8_t		type;

	union FieldData
	{
		GroupLink	link;
		MessageAddress	addr;
		int32_t		min1900;
		uint8_t		raw[1];

	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define COMMON_FIELD_HEADER_SIZE	(sizeof(Barry::CommonField) - sizeof(Barry::CommonField::FieldData))
#define COMMON_FIELD_MIN1900_SIZE	(sizeof(int32_t))


///////////////////////////////////////////////////////////////////////////////
// Address book / Contact record data

struct OldContactRecord
{
	uint32_t	uniqueId;
	uint8_t		unknown;
	CommonField	field[1];
} __attribute__ ((packed));
#define OLD_CONTACT_RECORD_HEADER_SIZE	(sizeof(Barry::OldContactRecord) - sizeof(Barry::CommonField))

struct ContactRecord
{
	uint32_t	uniqueId;
	uint8_t		unknown[3];
	CommonField	field[1];
} __attribute__ ((packed));
#define CONTACT_RECORD_HEADER_SIZE	(sizeof(Barry::ContactRecord) - sizeof(Barry::CommonField))


///////////////////////////////////////////////////////////////////////////////
// Message field and record structures

struct OldMessageRecord
{
	uint8_t		timeBlock[0x72];
	CommonField	field[1];
} __attribute__ ((packed));

struct MessageRecord
{
	uint8_t		timeBlock[0x74];
	CommonField	field[1];
} __attribute__ ((packed));



///////////////////////////////////////////////////////////////////////////////
// Calendar field and record structures

struct OldCalendarRecord
{
	uint32_t	uniqueId;
	uint8_t		unknown;	// observed as 0 or 1
	CommonField	field[1];
} __attribute__ ((packed));
#define OLD_CALENDAR_RECORD_HEADER_SIZE	(sizeof(Barry::OldCalendarRecord) - sizeof(Barry::CommonField))

struct CalendarRecord
{
	// FIXME - not yet implemented
	CommonField	field[1];
} __attribute__ ((packed));
//#define CALENDAR_RECORD_HEADER_SIZE	(sizeof(Barry::CalendarRecord) - sizeof(Barry::CommonField))



///////////////////////////////////////////////////////////////////////////////
// Service Book field and record structures

struct OldServiceBookRecord
{
	uint32_t	uniqueId;
	uint8_t		unknown;
	CommonField	field[1];
} __attribute__ ((packed));
#define OLD_SERVICE_BOOK_RECORD_HEADER_SIZE (sizeof(Barry::OldServiceBookRecord) - sizeof(Barry::CommonField))



///////////////////////////////////////////////////////////////////////////////
// Database access command structure

// even fragmented packets have a tableCmd
struct DBAccess
{
	uint8_t		tableCmd;
	union DBData
	{
		DBCommand		command;
		DBResponse		response;
		OldDBResponse		old_response;
		UploadCommand		upload;
		CommandTableField	table[1];
		OldDBDBRecord		old_dbdb;
		DBDBRecord		dbdb;
		uint8_t			return_code;

		uint8_t			fragment[1];

	} __attribute__ ((packed)) u;
} __attribute__ ((packed));



///////////////////////////////////////////////////////////////////////////////
// Main packet struct

struct Packet
{
	uint16_t	socket;		// socket ID... 0 is always there
	uint16_t	size;		// total size of data packet
	uint8_t		command;

	union PacketData
	{

		SocketCommand		socket;
		SequenceCommand		sequence;
		ModeSelectCommand	mode;
		DBAccess		db;
		uint8_t			raw[1];

	} __attribute__ ((packed)) u;
} __attribute__ ((packed));

// minimum required sizes for various responses
#define MIN_PACKET_SIZE		6


// maximum sizes
#define MAX_PACKET_SIZE		0x400	// anything beyond this needs to be
					// fragmented
// various useful sizes
#define SB_PACKET_HEADER_SIZE			(sizeof(Barry::Packet) - sizeof(Barry::Packet::PacketData))
#define SB_DBACCESS_HEADER_SIZE			(sizeof(Barry::DBAccess) - sizeof(Barry::DBAccess::DBData))
#define SB_PACKET_DBACCESS_HEADER_SIZE		(SB_PACKET_HEADER_SIZE + SB_DBACCESS_HEADER_SIZE)

#define SB_FRAG_HEADER_SIZE			SB_PACKET_DBACCESS_HEADER_SIZE
#define SB_SEQUENCE_PACKET_SIZE			(SB_PACKET_HEADER_SIZE + sizeof(Barry::SequenceCommand))
#define SB_SOCKET_PACKET_SIZE			(SB_PACKET_HEADER_SIZE + sizeof(Barry::SocketCommand))
#define SB_MODE_PACKET_COMMAND_SIZE		(SB_PACKET_HEADER_SIZE + sizeof(Barry::ModeSelectCommand) - sizeof(Barry::ModeSelectCommand::ResponseBlock))
#define SB_MODE_PACKET_RESPONSE_SIZE		(SB_PACKET_HEADER_SIZE + sizeof(Barry::ModeSelectCommand))
#define SB_PACKET_DBDB_HEADER_SIZE		(SB_PACKET_HEADER_SIZE + SB_DBACCESS_HEADER_SIZE + DBDB_RECORD_HEADER_SIZE)
#define SB_PACKET_OLD_DBDB_HEADER_SIZE		(SB_PACKET_HEADER_SIZE + SB_DBACCESS_HEADER_SIZE + OLD_DBDB_RECORD_HEADER_SIZE)

#define SB_PACKET_RESPONSE_HEADER_SIZE		(SB_PACKET_DBACCESS_HEADER_SIZE + DB_RESPONSE_HEADER_SIZE)
#define SB_PACKET_OLD_RESPONSE_HEADER_SIZE	(SB_PACKET_DBACCESS_HEADER_SIZE + OLD_DB_RESPONSE_HEADER_SIZE)
#define SB_PACKET_UPLOAD_HEADER_SIZE		(SB_PACKET_DBACCESS_HEADER_SIZE + UPLOAD_HEADER_SIZE)



// Macros
#define COMMAND(data)				(((const Barry::Packet *)data.GetData())->command)
#define IS_COMMAND(data, cmd)			(COMMAND(data) == cmd)
#define MAKE_PACKET(var, data)			const Barry::Packet *var = (const Barry::Packet *) data.GetData()
#define MAKE_PACKETPTR_BUF(var, ptr)		Barry::Packet *var = (Barry::Packet *)ptr
#define MAKE_RECORD(type,var,data,off)		type *var = (type *) (data.GetData() + (off))
#define MAKE_RECORD_PTR(type,var,data,off)	type *var = (type *) (data + (off))

// fragmentation protocol
// send DATA first, then keep sending DATA packets, FRAGMENTing
// as required until finished, then send DONE.  Both sides behave
// this way, so different sized data can be sent in both
// directions
//
// the fragmented piece only has a the param header, and then continues
// right on with the data



// checks packet size and throws BError if not right
void CheckSize(const Data &packet, size_t requiredsize = MIN_PACKET_SIZE);

} // namespace Barry

#endif

