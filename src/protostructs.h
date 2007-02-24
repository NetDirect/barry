///
/// \file	protostructs.h
///		USB Blackberry bulk protocol API.  This is split out from
///		protocol.h so that low level, packed structs can be
///		compiled separately from the application.  This prevents
///		aliasing problems in the application, or using
///		-fno-strict-aliasing, which the library only needs.
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

#ifndef __BARRY_PROTOSTRUCTS_H__
#define __BARRY_PROTOSTRUCTS_H__

#include <stdint.h>
#include <sys/types.h>

// forward declarations
namespace Barry { class Data; }

namespace Barry { namespace Protocol {

///////////////////////////////////////////////////////////////////////////////
union SizePacket
{
	uint16_t size;
	char buffer[4];
} __attribute__ ((packed));


///////////////////////////////////////////////////////////////////////////////
// Record sub-field structs

struct GroupLink				// used for Contacts records
{
	uint32_t	uniqueId;
	uint16_t	unknown;
} __attribute__ ((packed));

struct MessageAddress				// used for Message records
{
	uint8_t		unknown[8];
	uint8_t		addr[1];	// 2 null terminated strings: first
					// contains full name, second contains
					// the email address
} __attribute__ ((packed));



///////////////////////////////////////////////////////////////////////////////
// Record Field Formats

struct CommonField
{
	uint16_t	size;			// including null terminator
	uint8_t		type;

	union CommonFieldData
	{

		GroupLink	link;
		MessageAddress	addr;
		int32_t		min1900;
		uint16_t	code;
		uint8_t		raw[1];

	} __attribute__ ((packed)) u;

} __attribute__ ((packed));
#define COMMON_FIELD_HEADER_SIZE	(sizeof(Barry::Protocol::CommonField) - sizeof(Barry::Protocol::CommonField::CommonFieldData))
#define COMMON_FIELD_MIN1900_SIZE	(sizeof(int32_t))

struct CommandTableField
{
	uint8_t		size;		// no null terminator
	uint8_t		code;
	uint8_t		name[1];
} __attribute__ ((packed));
#define COMMAND_FIELD_HEADER_SIZE	(sizeof(Barry::Protocol::CommandTableField) - 1)

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
#define OLD_DBDB_FIELD_HEADER_SIZE	(sizeof(Barry::Protocol::OldDBDBField) - 1)

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
#define DBDB_FIELD_HEADER_SIZE	(sizeof(Barry::Protocol::DBDBField) - 1)

struct RecordStateTableField
{
	uint8_t		rectype;		// it is unknown exactly what
						// this field does, but it
						// shows up here and in the
						// tagged record header, and
						// for some of the records
						// they must match when writing
	uint16_t	index;
	uint32_t	uniqueId;		// matches the uniqueId of say,
						// address book records
	uint8_t		flags;			// bit 0x01 is the dirty flag
						// don't know if any other bits
						// are used
#define BARRY_RSTF_DIRTY	0x01
	uint8_t		unknown2[4];
} __attribute__ ((packed));

struct CalendarRecurranceDataField  // as documented in the Cassis project spec
{
	uint8_t		type;
#define CRDF_TYPE_DAY		0x01
#define CRDF_TYPE_MONTH_BY_DATE	0x03
#define CRDF_TYPE_MONTH_BY_DAY	0x04
#define CRDF_TYPE_YEAR_BY_DATE	0x05
#define CRDF_TYPE_YEAR_BY_DAY	0x06
#define CRDF_TYPE_WEEK		0x0c

	uint8_t		unknown;		// always 0x01
	uint16_t	interval;
	uint32_t	startTime;
	uint32_t	endTime;		// 0xFFFFFFFF for never

	union Additional
	{
		// Note: blank fields should be set to 0

		struct Day
		{
			uint8_t day[6];		// always zeros!
		} __attribute__ ((packed)) day;

		struct MonthByDate
		{
			uint8_t monthDay;	// day of month to recur on
						// (1-31)
			uint8_t blank[5];
		} __attribute__ ((packed)) month_by_date;

		struct MonthByDay
		{
			uint8_t weekDay;	// day of week to recur on (0-6)
			uint8_t week;		// week of month to recur on
						// (1 to 5, first week, second
						// week, etc)
			uint8_t blank[4];
		} __attribute__ ((packed)) month_by_day;

		struct YearByDate
		{
			uint8_t monthDay;	// day of month to recur on
						// (1-31)
			uint8_t blank;
			uint8_t month;		// month to recur on (1-12)
			uint8_t blank_[3];
		} __attribute__ ((packed)) year_by_date;

		struct YearByDay
		{
			uint8_t weekDay;	// day of week to recur on (0-6)
			uint8_t week;		// week of month (1 to 5)
			uint8_t month;		// (1-12)
			uint8_t blank[3];
		} __attribute__ ((packed)) year_by_day;

		struct Week
		{
			uint8_t	days;		// bitmask
			#define CRDF_WD_SUN	0x01
			#define CRDF_WD_MON	0x02
			#define CRDF_WD_TUE	0x04
			#define CRDF_WD_WED	0x08
			#define CRDF_WD_THU	0x10
			#define CRDF_WD_FRI	0x20
			#define CRDF_WD_SAT	0x40

			uint8_t blank[5];
		} __attribute__ ((packed)) week;

	} __attribute__ ((packed)) u;

} __attribute__ ((packed));
#define CALENDAR_RECURRANCE_DATA_FIELD_SIZE	sizeof(Barry::Protocol::CalendarRecurranceDataField)



///////////////////////////////////////////////////////////////////////////////
// Packed field structures - odd format used with Service Book records

struct PackedField_02
{
	uint8_t		code;
	uint8_t		size;
	uint8_t		type;
	uint8_t		raw[1];
} __attribute__ ((packed));
#define PACKED_FIELD_02_HEADER_SIZE	(sizeof(Barry::Protocol::PackedField_02) - 1)

struct PackedField_10
{
	uint8_t		type;
	uint8_t		size;
	uint8_t		raw[1];
} __attribute__ ((packed));
#define PACKED_FIELD_10_HEADER_SIZE	(sizeof(Barry::Protocol::PackedField_10) - 1)




///////////////////////////////////////////////////////////////////////////////
// Service Book field and record structures

struct ServiceBookConfigField
{
	uint8_t		format;
	uint8_t		fields[1];
} __attribute__ ((packed));
#define SERVICE_BOOK_CONFIG_FIELD_HEADER_SIZE (sizeof(Barry::Protocol::ServiceBookConfigField) - 1)


///////////////////////////////////////////////////////////////////////////////
// DB Command Parameter structures

struct DBC_Record
{
	uint16_t	recordIndex;	// index comes from RecordStateTable
	uint8_t		data[1];
} __attribute__ ((packed));
#define DBC_RECORD_HEADER_SIZE		(sizeof(Barry::Protocol::DBC_Record) - 1)

struct DBC_RecordFlags
{
	uint8_t		unknown;
	uint16_t	index;
	uint8_t		unknown2[5];
} __attribute__ ((packed));
#define DBC_RECORD_FLAGS_SIZE		(sizeof(Barry::Protocol::DBC_RecordFlags))

struct DBC_TaggedUpload
{
	uint8_t		rectype;		// it is unknown exactly what
						// this field does, but it
						// shows up here and in the
						// RecordStateTable, and
						// for some of the records
						// they must match when writing
	uint32_t	uniqueId;
	uint8_t		unknown2;
	uint8_t		data[1];
} __attribute__ ((packed));
#define DBC_TAGGED_UPLOAD_HEADER_SIZE	(sizeof(Barry::Protocol::DBC_TaggedUpload) - 1)

struct DBC_IndexedUpload
{
	uint8_t		unknown;	// observed: 00 or 05
	uint16_t	index;
	uint8_t		data[1];
} __attribute__ ((packed));
#define DBC_INDEXED_UPLOAD_HEADER_SIZE	(sizeof(Barry::Protocol::DBC_IndexedUpload) - 1)

struct PasswordChallenge
{
	uint8_t		remaining_tries;	// number of password attempts
						// the device will accept before
						// committing suicide...
						// starts at 10 and counts down
						// on each bad password
	uint8_t		unknown;		// observed as 0... probably just
						// the top byte of a uint16
						// remaining_tries, but I don't
						// want to take that chance
	uint16_t	param;			// seems to be a secondary command
						// of some kind, observed as 0x14
						// or 0x04, but purpose unknown
						// possibly a send/receive flag
						// bit (0x10/0x00)
	union Hash
	{
		uint32_t	seed;
		uint8_t		hash[20];
	} __attribute__ ((packed)) u;

} __attribute__ ((packed));

struct AttributeFetch
{
	uint16_t	object;
	uint16_t	attribute;
	uint8_t		raw[1];			// used only in response
} __attribute__ ((packed));
#define ATTRIBUTE_FETCH_COMMAND_SIZE	(sizeof(Barry::Protocol::AttributeFetch) - 1)

struct ModeSelect
{
	uint8_t		name[16];
	struct ResponseBlock
	{
		uint8_t		unknown[20];
	} __attribute__ ((packed)) response;
} __attribute__ ((packed));


///////////////////////////////////////////////////////////////////////////////
// Protocol command structures

struct SocketCommand
{
	uint16_t	socket;
	uint8_t		sequence;		// incremented on each socket 0
						// communication, replies return
						// the same number from command

	union PacketData
	{

		PasswordChallenge	password;
		AttributeFetch		fetch;
		ModeSelect		mode;
		uint8_t			raw[1];

	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define SOCKET_COMMAND_HEADER_SIZE		(sizeof(Barry::Protocol::SocketCommand) - sizeof(Barry::Protocol::SocketCommand::PacketData))

struct SequenceCommand
{
	uint8_t		unknown1;
	uint8_t		unknown2;
	uint8_t		unknown3;
	uint32_t	sequenceId;
} __attribute__ ((packed));

struct DBCommand
{
	uint8_t		operation;	// see below
	uint16_t	databaseId;	// value from the Database Database

	union Parameters
	{

		DBC_Record		record;
		DBC_RecordFlags		flags;
		DBC_TaggedUpload	tag_upload;
		DBC_IndexedUpload	index_upload;
		uint8_t			raw[1];

	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define DB_COMMAND_HEADER_SIZE		(sizeof(Barry::Protocol::DBCommand) - sizeof(Barry::Protocol::DBCommand::Parameters))



///////////////////////////////////////////////////////////////////////////////
// Protocol response parameter structures

struct DBR_OldDBDBRecord
{
	uint16_t	count;			// number of fields in record
	OldDBDBField	field[1];
} __attribute__ ((packed));
#define OLD_DBDB_RECORD_HEADER_SIZE	(sizeof(Barry::Protocol::DBR_OldDBDBRecord) - sizeof(Barry::Protocol::OldDBDBField))

struct DBR_DBDBRecord
{
	uint16_t	count;
	uint8_t		unknown[3];
	DBDBField	field[1];
} __attribute__ ((packed));
#define DBDB_RECORD_HEADER_SIZE		(sizeof(Barry::Protocol::DBR_DBDBRecord) - sizeof(Barry::Protocol::DBDBField))

// Records with a uniqueId.  This covers the following records:
//
//	Old Contact records
//	Old Service Book records
//	Old Calendar records
//
struct DBR_OldTaggedRecord
{
	uint8_t		rectype;
	uint16_t	index;
	uint32_t	uniqueId;
	uint8_t		unknown2;

	union TaggedData
	{
		CommonField	field[1];
	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define DBR_OLD_TAGGED_RECORD_HEADER_SIZE (sizeof(Barry::Protocol::DBR_OldTaggedRecord) - sizeof(Barry::Protocol::DBR_OldTaggedRecord::TaggedData))

struct MessageRecord
{
	uint8_t		timeBlock[0x74];
	CommonField	field[1];
} __attribute__ ((packed));
#define MESSAGE_RECORD_HEADER_SIZE (sizeof(Barry::Protocol::MessageRecord) - sizeof(Barry::Protocol::CommonField))



///////////////////////////////////////////////////////////////////////////////
// Protocol response structures

struct DBResponse
{
	uint8_t		operation;

	union Parameters
	{

		DBR_OldTaggedRecord	tagged;
		DBR_OldDBDBRecord	old_dbdb;
		DBR_DBDBRecord		dbdb;

	} __attribute__ ((packed)) u;

} __attribute__ ((packed));
#define DB_RESPONSE_HEADER_SIZE		(sizeof(Barry::Protocol::DBResponse) - sizeof(Barry::Protocol::DBResponse::Parameters))



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
		CommandTableField	table[1];
		uint8_t			return_code;
		uint8_t			fragment[1];

	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define SB_DBACCESS_HEADER_SIZE			(sizeof(Barry::Protocol::DBAccess) - sizeof(Barry::Protocol::DBAccess::DBData))
#define SB_DBACCESS_RETURN_CODE_SIZE		(1)



///////////////////////////////////////////////////////////////////////////////
// Main packet struct

struct Packet
{
	uint16_t	socket;		// socket ID... 0 exists by default
	uint16_t	size;		// total size of data packet
	uint8_t		command;

	union PacketData
	{

		SocketCommand		socket;
		SequenceCommand		sequence;
		DBAccess		db;
		uint8_t			raw[1];

	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define SB_PACKET_HEADER_SIZE			(sizeof(Barry::Protocol::Packet) - sizeof(Barry::Protocol::Packet::PacketData))

// minimum required sizes for various responses
#define MIN_PACKET_SIZE		6


// maximum sizes
#define MAX_PACKET_SIZE		0x400	// anything beyond this needs to be
					// fragmented

/////////////////////////////////////////////////////////////////////////////
//
// various useful sizes
//

#define SB_PACKET_DBACCESS_HEADER_SIZE		(SB_PACKET_HEADER_SIZE + SB_DBACCESS_HEADER_SIZE)
#define SB_FRAG_HEADER_SIZE			SB_PACKET_DBACCESS_HEADER_SIZE

#define SB_PACKET_COMMAND_HEADER_SIZE		(SB_PACKET_DBACCESS_HEADER_SIZE + DB_COMMAND_HEADER_SIZE)
#define SB_PACKET_RESPONSE_HEADER_SIZE		(SB_PACKET_DBACCESS_HEADER_SIZE + DB_RESPONSE_HEADER_SIZE)

#define SB_PACKET_DBDB_HEADER_SIZE		(SB_PACKET_RESPONSE_HEADER_SIZE + DBDB_RECORD_HEADER_SIZE)
#define SB_PACKET_OLD_DBDB_HEADER_SIZE		(SB_PACKET_RESPONSE_HEADER_SIZE + OLD_DBDB_RECORD_HEADER_SIZE)

#define SB_PACKET_UPLOAD_HEADER_SIZE		(SB_PACKET_DBACCESS_HEADER_SIZE + UPLOAD_HEADER_SIZE)

#define SB_SEQUENCE_PACKET_SIZE			(SB_PACKET_HEADER_SIZE + sizeof(Barry::Protocol::SequenceCommand))
#define SB_SOCKET_PACKET_HEADER_SIZE		(SB_PACKET_HEADER_SIZE + SOCKET_COMMAND_HEADER_SIZE)
#define SB_MODE_PACKET_COMMAND_SIZE		(SB_SOCKET_PACKET_HEADER_SIZE + sizeof(Barry::Protocol::ModeSelect) - sizeof(Barry::Protocol::ModeSelect::ResponseBlock))
#define SB_MODE_PACKET_RESPONSE_SIZE		(SB_SOCKET_PACKET_HEADER_SIZE + sizeof(Barry::Protocol::ModeSelect))


// Macros
#define COMMAND(data)				(((const Barry::Protocol::Packet *)data.GetData())->command)
#define IS_COMMAND(data, cmd)			(COMMAND(data) == cmd)
#define MAKE_PACKET(var, data)			const Barry::Protocol::Packet *var = (const Barry::Protocol::Packet *) data.GetData()
#define MAKE_PACKETPTR_BUF(var, ptr)		Barry::Protocol::Packet *var = (Barry::Protocol::Packet *)ptr
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
void CheckSize(const Barry::Data &packet, size_t requiredsize = MIN_PACKET_SIZE);
unsigned int GetSize(const Barry::Data &packet);

}} // namespace Barry::Protocol

#endif

