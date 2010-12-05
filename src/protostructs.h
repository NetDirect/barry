///
/// \file	protostructs.h
///		USB Blackberry bulk protocol API.  This is split out from
///		protocol.h so that low level, packed structs can be
///		compiled separately from the application.  This prevents
///		aliasing problems in the application, or using
///		-fno-strict-aliasing, which the library only needs.
///
///		Do not include this in any Barry library header.
///		This may only be included from .cc files, in order
///		to hide aliasing concernes from the application.
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
// SMS Message field and record structures

struct SMSMetaData
{
	uint8_t		recv; // if received, this is set to 1; otherwise 0
	uint8_t		flags;
#define SMS_FLG_NEW_CONVERSATION 0x20
#define SMS_FLG_SAVED 0x10
#define SMS_FLG_DELETED 0x08
#define SMS_FLG_OPENED 0x01

	uint8_t		new_flag;
	uint16_t	zero; // constantly 0
	uint32_t	status;
#define SMS_STA_RECEIVED 0x000007ff
#define SMS_STA_DRAFT 0x7fffffff

	uint32_t	error_id;
	uint64_t	timestamp;
	uint64_t	service_center_timestamp;
	uint8_t		dcs;
#define SMS_DCS_7BIT 0x00
#define SMS_DCS_8BIT 0x01
#define SMS_DCS_UCS2 0x02

} __attribute__ ((packed));
#define SMS_METADATA_SIZE	(sizeof(::Barry::Protocol::SMSMetaData))



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
		SMSMetaData	sms_metadata;
		uint64_t	timestamp;
		uint64_t	uint64;
		uint32_t	uint32;
		int32_t		min1900;
		uint16_t	code;
		uint8_t		raw[1];
		int16_t		int16;

	} __attribute__ ((packed)) u;

} __attribute__ ((packed));
#define COMMON_FIELD_HEADER_SIZE	(sizeof(::Barry::Protocol::CommonField) - sizeof(::Barry::Protocol::CommonField::CommonFieldData))
#define COMMON_FIELD_MIN1900_SIZE	(sizeof(int32_t))

struct CommandTableField
{
	uint8_t		size;		// no null terminator
	uint8_t		code;
	uint8_t		name[1];
} __attribute__ ((packed));
#define COMMAND_FIELD_HEADER_SIZE	(sizeof(::Barry::Protocol::CommandTableField) - 1)

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
#define OLD_DBDB_FIELD_HEADER_SIZE	(sizeof(::Barry::Protocol::OldDBDBField) - 1)

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
#define DBDB_FIELD_HEADER_SIZE	(sizeof(::Barry::Protocol::DBDBField) - 1)

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

struct CalendarRecurrenceDataField  // as documented in the Cassis project spec
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
#define CALENDAR_RECURRENCE_DATA_FIELD_SIZE	sizeof(::Barry::Protocol::CalendarRecurrenceDataField)

//
// Calendar record: field constants
//

#define CR_FREEBUSY_FREE		0
#define CR_FREEBUSY_TENTATIVE		1
#define CR_FREEBUSY_BUSY		2
#define CR_FREEBUSY_OUT_OF_OFFICE	3
#define CR_FREEBUSY_RANGE_LOW		0
#define CR_FREEBUSY_RANGE_HIGH		3

#define CR_CLASS_PUBLIC			0
#define CR_CLASS_CONFIDENTIAL		1
#define CR_CLASS_PRIVATE		2
#define CR_CLASS_RANGE_LOW		0
#define CR_CLASS_RANGE_HIGH		2


//
// Task record: field constants
//

#define TR_ALARM_DATE			1
#define TR_ALARM_RELATIVE		2
#define TR_ALARM_RANGE_LOW		1
#define TR_ALARM_RANGE_HIGH		2

#define TR_PRIORITY_HIGH		0
#define TR_PRIORITY_NORMAL		1
#define TR_PRIORITY_LOW			2
#define TR_PRIORITY_RANGE_LOW		0
#define TR_PRIORITY_RANGE_HIGH		2

#define TR_STATUS_NOT_STARTED		0
#define TR_STATUS_IN_PROGRESS		1
#define TR_STATUS_COMPLETED		2
#define TR_STATUS_WAITING		3
#define TR_STATUS_DEFERRED		4
#define TR_STATUS_RANGE_LOW		0
#define TR_STATUS_RANGE_HIGH		4

//
// Phone Call Logs record: field constants
//
//
#define CLL_DIRECTION_RECEIVER		0
#define CLL_DIRECTION_EMITTER		1
#define CLL_DIRECTION_FAILED		2
#define CLL_DIRECTION_MISSING		3
#define CLL_DIRECTION_RANGE_LOW		0
#define CLL_DIRECTION_RANGE_HIGH	3

#define CLL_PHONETYPE_UNDEFINED		0
#define CLL_PHONETYPE_OFFICE		1
#define CLL_PHONETYPE_HOME			2
#define CLL_PHONETYPE_MOBILE		3
#define CLL_PHONETYPE_RANGE_LOW		0
#define CLL_PHONETYPE_RANGE_HIGH	3

//
// Browser Bookmarks record: field constants
//
//
#define BMK_BROWSER_AUTO				0
#define BMK_BROWSER_BLACKBERRY			1
#define BMK_BROWSER_FIREFOX				2
#define BMK_BROWSER_INTERNET_EXPLORER	3

#define BMK_DISPLAY_AUTO				0
#define BMK_DISPLAY_COLOMN				1
#define BMK_DISPLAY_PAGE				2

#define BMK_JAVASCRIPT_AUTO				0
#define BMK_JAVASCRIPT_ENABLED			1
#define BMK_JAVASCRIPT_DISABLED			2

//
// Folder record: field constants
//

#define FR_TYPE_SUBTREE			0x00
#define FR_TYPE_DELETED			0x01
#define FR_TYPE_INBOX			0x02
#define FR_TYPE_OUTBOX			0x03
#define FR_TYPE_SENT			0x04
#define FR_TYPE_OTHER			0x05
#define FR_TYPE_DRAFT			0x0a

#define FR_STATUS_ORPHAN		0x50
#define FR_STATUS_UNFILED		0x51
#define FR_STATUS_FILED			0x52


///////////////////////////////////////////////////////////////////////////////
// Packed field structures - odd format used with Service Book records

struct PackedField_02
{
	uint8_t		code;
	uint8_t		size;
	uint8_t		type;
	uint8_t		raw[1];
} __attribute__ ((packed));
#define PACKED_FIELD_02_HEADER_SIZE	(sizeof(::Barry::Protocol::PackedField_02) - 1)

struct PackedField_10
{
	uint8_t		type;
	uint8_t		size;
	uint8_t		raw[1];
} __attribute__ ((packed));
#define PACKED_FIELD_10_HEADER_SIZE	(sizeof(::Barry::Protocol::PackedField_10) - 1)




///////////////////////////////////////////////////////////////////////////////
// Service Book field and record structures

struct ServiceBookConfigField
{
	uint8_t		format;
	uint8_t		fields[1];
} __attribute__ ((packed));
#define SERVICE_BOOK_CONFIG_FIELD_HEADER_SIZE (sizeof(::Barry::Protocol::ServiceBookConfigField) - 1)


///////////////////////////////////////////////////////////////////////////////
// DB Command Parameter structures

struct DBC_Record
{
	uint16_t	recordIndex;	// index comes from RecordStateTable
	uint8_t		data[1];
} __attribute__ ((packed));
#define DBC_RECORD_HEADER_SIZE		(sizeof(::Barry::Protocol::DBC_Record) - 1)

struct DBC_RecordFlags
{
	uint8_t		unknown;
	uint16_t	index;
	uint8_t		unknown2[5];
} __attribute__ ((packed));
#define DBC_RECORD_FLAGS_SIZE		(sizeof(::Barry::Protocol::DBC_RecordFlags))

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
#define DBC_TAGGED_UPLOAD_HEADER_SIZE	(sizeof(::Barry::Protocol::DBC_TaggedUpload) - 1)

struct DBC_IndexedUpload
{
	uint8_t		unknown;	// observed: 00 or 05
	uint16_t	index;
	uint8_t		data[1];
} __attribute__ ((packed));
#define DBC_INDEXED_UPLOAD_HEADER_SIZE	(sizeof(::Barry::Protocol::DBC_IndexedUpload) - 1)

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
#define PASSWORD_CHALLENGE_HEADER_SIZE	(sizeof(::Barry::Protocol::PasswordChallenge) - sizeof(::Barry::Protocol::PasswordChallenge::Hash))
#define PASSWORD_CHALLENGE_SEED_SIZE	(PASSWORD_CHALLENGE_HEADER_SIZE + sizeof(uint32_t))
#define PASSWORD_CHALLENGE_SIZE		(sizeof(::Barry::Protocol::PasswordChallenge))

struct AttributeFetch
{
	uint16_t	object;
	uint16_t	attribute;
	uint8_t		raw[1];			// used only in response
} __attribute__ ((packed));
#define ATTRIBUTE_FETCH_COMMAND_SIZE	(sizeof(::Barry::Protocol::AttributeFetch) - 1)

struct ModeSelect
{
	uint8_t		name[16];
	struct ResponseBlock
	{
		uint8_t		unknown[20];
	} __attribute__ ((packed)) response;
} __attribute__ ((packed));

struct Echo
{
	uint64_t	ticks;			// number of microseconds since
						// host system startup
} __attribute__ ((packed));
#define ECHO_COMMAND_SIZE		(sizeof(::Barry::Protocol::Echo))


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
		Echo			echo;

	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define SOCKET_COMMAND_HEADER_SIZE		(sizeof(::Barry::Protocol::SocketCommand) - sizeof(::Barry::Protocol::SocketCommand::PacketData))

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
#define DB_COMMAND_HEADER_SIZE		(sizeof(::Barry::Protocol::DBCommand) - sizeof(::Barry::Protocol::DBCommand::Parameters))



///////////////////////////////////////////////////////////////////////////////
// Protocol response parameter structures

struct DBR_OldDBDBRecord
{
	uint16_t	count;			// number of fields in record
	OldDBDBField	field[1];
} __attribute__ ((packed));
#define OLD_DBDB_RECORD_HEADER_SIZE	(sizeof(::Barry::Protocol::DBR_OldDBDBRecord) - sizeof(::Barry::Protocol::OldDBDBField))

struct DBR_DBDBRecord
{
	uint16_t	count;
	uint8_t		unknown[3];
	DBDBField	field[1];
} __attribute__ ((packed));
#define DBDB_RECORD_HEADER_SIZE		(sizeof(::Barry::Protocol::DBR_DBDBRecord) - sizeof(::Barry::Protocol::DBDBField))

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
#define DBR_OLD_TAGGED_RECORD_HEADER_SIZE (sizeof(::Barry::Protocol::DBR_OldTaggedRecord) - sizeof(::Barry::Protocol::DBR_OldTaggedRecord::TaggedData))

struct MessageRecord
{
	uint8_t		field1;		// always 'j'
	uint32_t	field2;		// always 0x00000000
	uint32_t	flags;		// flags
	uint32_t	field4;		// normal email and pin recv this is 0x7ff
					// changes on sent and reply to 0x01ffffff
					// and 0x003fffff on pin send
	uint32_t	field5;		// always 0x00000000
	uint32_t	field6;		// always 0x00000000
	uint32_t	field7;		// always 0x00000000
	uint32_t	field8;		// always 0x00000000
	uint16_t	field9;		// always 0x0000

	uint16_t	dateReceived;	// the first two of these time fields are always the same
	uint16_t	timeReceived;	//
	uint16_t	dateDuplicate;	// On mail sent from the BB all three fields are identical
	uint16_t	timeDuplicate;	// (time sent)
	uint16_t	dateSent;
	uint16_t	timeSent;

	uint16_t	priority;	// priority field
	uint32_t	field14;	// always 0x00000000
	uint32_t	field15;	// always 0x00000000
	uint16_t	field16;	// always 0x0000
	uint32_t	field13;	// PIN reply 0x00000000 other time 0xffffffff or 0xfffffffe
	uint16_t	messageSize;	// Message size, 0x0000 if Reply or Saved, 0xffff if below ????
	uint32_t	field18;	// 0x0's and 0xF'x
	uint32_t	field19;	// 0x0's and 0xF's
	uint16_t	field20;	// always 0x0000
	uint16_t	field21;	// 0x01 unless PIN reply then 0x00
	uint32_t	inReplyTo;	// reply to message?
	uint32_t	field22;	// always 0x00000000
	uint16_t	field23;	// FIXME

	uint32_t	folderOne;	// these are the 'folders' the message is in
	uint32_t	folderTwo;	//

	uint16_t	replyMessageFlags;	// 0xfffe on recvd messages
					// 0x001b on reply
					// 0x0015 on send
					// 0x3 pin send
					// 0x2 on pin recv
	uint16_t	field27;	// set to 0x00000004 on PIN reply, 0x00000005 otherwise
	uint32_t	headerUID;	// yet another copy of the UID (RecId)

	uint32_t	field29;	// always 0x00000000
	uint16_t	field30;	// always 0x0002
	uint16_t	field31;	// always 0x00000000
	uint16_t	field32;	// always 0x0004
	uint16_t	field34;	// always 0x0000
	uint8_t		field33;	// always 'd'
	uint32_t	timeBlock;	// FIXME
	CommonField	field[1];
} __attribute__ ((packed));
#define MESSAGE_RECORD_HEADER_SIZE (sizeof(::Barry::Protocol::MessageRecord) - sizeof(::Barry::Protocol::CommonField))



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
#define DB_RESPONSE_HEADER_SIZE		(sizeof(::Barry::Protocol::DBResponse) - sizeof(::Barry::Protocol::DBResponse::Parameters))



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
#define SB_DBACCESS_HEADER_SIZE			(sizeof(::Barry::Protocol::DBAccess) - sizeof(::Barry::Protocol::DBAccess::DBData))
#define SB_DBACCESS_RETURN_CODE_SIZE		(1)



///////////////////////////////////////////////////////////////////////////////
// Javaloader protocol structure

struct JLDirEntry
{
	uint16_t	unknown;
	uint32_t	timestamp;
	uint16_t	filename_size;
	uint8_t		filename[1];
	// the rest of the packet is variable length
	// another string for version, then:
	// uint32_t	cod_size;

} __attribute__ ((packed));
#define SB_JLDIRENTRY_HEADER_SIZE		(sizeof(::Barry::Protocol::JLDirEntry) - 1)

struct JLCommand
{
	uint8_t		command;
	uint8_t		unknown;	// nearly always 0, might be top half of command
	uint16_t	size;
} __attribute__ ((packed));
#define SB_JLCOMMAND_HEADER_SIZE		(sizeof(::Barry::Protocol::JLCommand))

struct JLResponse
{
	uint8_t		command;
	uint8_t		unknown;
	uint16_t	expect;
} __attribute__ ((packed));
#define SB_JLRESPONSE_HEADER_SIZE		(sizeof(::Barry::Protocol::JLResponse))

struct JLScreenInfo
{
	uint16_t	unknown1;
	uint16_t	unknown2;
	uint16_t	unknown3;
	uint16_t	width;
	uint16_t	height;
	uint16_t	unknown4;
	uint16_t	unknown5;
	uint16_t	unknown6;
} __attribute__ ((packed));
#define SB_JLSCREENINFO_SIZE			(sizeof(::Barry::Protocol::JLScreenInfo))

struct JLEventlogEntry
{
	uint16_t	size;
	// remainder of packet is variable
	// it contains the log data as an ASCII (UTF-8?) string
} __attribute__ ((packed));
#define SB_JLEVENTLOG_ENTRY_HEADER_SIZE		(sizeof(::Barry::Protocol::JLEventlogEntry))

struct JLDeviceInfo
{
	uint32_t	hardware_id;
	uint32_t	pin;
	uint32_t	os_version;
	uint32_t	vm_version;
	uint32_t	radio_id;
	uint32_t	vendor_id;
	uint32_t	active_wafs;
	// older devices (such as 7130) don't this extra data in the
	// device info packet and will therefore fail the size check
	//uint8_t		raw[4];
} __attribute__ ((packed));
#define SB_JLDEVICEINFO_SIZE			(sizeof(::Barry::Protocol::JLDeviceInfo))

struct JLPacket
{
	uint16_t	socket;
	uint16_t	size;		// total size of data packet

	union PacketData
	{
		JLCommand		command;
		JLResponse		response;
		JLScreenInfo		screeninfo;
		JLEventlogEntry		logentry;
		JLDeviceInfo		devinfo;
		uint8_t			raw[1];
		char			filename[1];
		uint32_t		cod_size;
		uint32_t		timestamp;
		uint16_t		id;
	} __attribute__ ((packed)) u;

} __attribute__ ((packed));
#define SB_JLPACKET_HEADER_SIZE		(sizeof(::Barry::Protocol::JLPacket) - sizeof(::Barry::Protocol::JLPacket::PacketData))


///////////////////////////////////////////////////////////////////////////////
// JavaDebug protocol structures

namespace JDWP {

	// Packet command
	//----------------

	struct PacketEventRequestSet {
		uint8_t eventKind;
		uint8_t suspendPolicy;
		uint32_t modifiers;
	} __attribute__ ((packed));


	struct PacketEventRequest {
		union PacketEventRequestData {
			PacketEventRequestSet set;
		} __attribute__ ((packed)) u;
	} __attribute__ ((packed));


	struct PacketCommand {
		uint8_t commandset;
		uint8_t command;

		union PacketCommandData {
			PacketEventRequest eventRequest;
		} __attribute__ ((packed)) u;
	} __attribute__ ((packed));
	#define JDWP_COMMAND_HEADER_SIZE			(sizeof(::Barry::Protocol::JDWP::PacketCommand))


	// Packet response
	//-----------------

	struct PacketVirtualMachineIDSizes {
		uint32_t fieldIDSize;
		uint32_t methodIDSize;
		uint32_t objectIDSize;
		uint32_t referenceTypeIDSize;
		uint32_t frameIDSize;
	} __attribute__ ((packed));

	#define JDWP_PACKETVIRTUALMACHINEIDSIZES_DATA_SIZE		sizeof(::Barry::Protocol::JDWP::PacketVirtualMachineIDSizes)


	struct PacketVirtualMachine {
		union PacketVirtualMachineData {
			PacketVirtualMachineIDSizes IDSizes;
		} __attribute__ ((packed)) u;
	} __attribute__ ((packed));


	struct PacketResponse {
		uint16_t errorcode;

		union PacketResponseData {
			PacketVirtualMachine virtualMachine;
			uint32_t value;
			uint8_t raw[1];
		} __attribute__ ((packed)) u;
	} __attribute__ ((packed));
	#define JDWP_RESPONSE_HEADER_SIZE			(sizeof(::Barry::Protocol::JDWP::PacketResponse) - sizeof(::Barry::Protocol::JDWP::PacketResponse::PacketResponseData))


	// Generic packet
	//----------------

	struct Packet {
		uint32_t length;
		uint32_t id;
		uint8_t flags;

		union PacketType {
			PacketCommand command;
			PacketResponse response;
		} __attribute__ ((packed)) u;
	} __attribute__ ((packed));
	#define JDWP_PACKET_HEADER_SIZE			(sizeof(::Barry::Protocol::JDWP::Packet) - sizeof(::Barry::Protocol::JDWP::Packet::PacketType))


	#define MAKE_JDWPPACKET(var, data)		const ::Barry::Protocol::JDWP::Packet *var = (const ::Barry::Protocol::JDWP::Packet *) (data).GetData()
	#define MAKE_JDWPPACKETPTR_BUF(var, ptr)		::Barry::Protocol::JDWP::Packet *var = (::Barry::Protocol::JDWP::Packet *)ptr


} // namespace JDWP

struct JDWField {
	uint32_t size;

	union JDWFieldData {
		uint8_t raw[1];
	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define JDWP_FIELD_HEADER_SIZE			(sizeof(::Barry::Protocol::JDWField) - sizeof(::Barry::Protocol::JDWField::JDWFieldData))

struct JVMCommand
{
	uint16_t	size;
	uint8_t		command;
	uint8_t		raw[1];
} __attribute__ ((packed));
#define SB_JVMCOMMAND_HEADER_SIZE		(sizeof(::Barry::Protocol::JVMCommand))

struct JVMResponse
{
	uint8_t		command;
	uint8_t		unknown;
	uint16_t	expect;
} __attribute__ ((packed));
#define SB_JVMRESPONSE_HEADER_SIZE		(sizeof(::Barry::Protocol::JVMResponse))

struct JVMModulesList
{
	uint32_t	nbr;
	// remainder of packet is variable
	// it contains the modules list 
} __attribute__ ((packed));
#define SB_JVMMODULES_LIST_HEADER_SIZE		(sizeof(::Barry::Protocol::JVMModulesList))

struct JVMModulesEntry
{
	uint32_t	id;
	uint32_t	uniqueId;
	uint16_t	sizename;
	// remainder of packet is variable
	// it contains the module name
} __attribute__ ((packed));
#define SB_JVMMODULES_ENTRY_HEADER_SIZE		(sizeof(::Barry::Protocol::JVMModulesEntry))

struct JVMThreadsList
{
	uint32_t	nbr;
	// remainder of packet is variable
	// it contains the threads list 
} __attribute__ ((packed));
#define SB_JVMTHREADS_LIST_HEADER_SIZE		(sizeof(::Barry::Protocol::JVMThreadsList))

struct JVMUnknown01
{
	uint8_t		byte;
	uint32_t	address;
} __attribute__ ((packed));
#define SB_JVMUNKNOWN01_HEADER_SIZE			(sizeof(::Barry::Protocol::JVMUnknown01))

struct JVMUnknown02
{
	uint32_t	address1;
	uint32_t	address2;
} __attribute__ ((packed));
#define SB_JVMUNKNOWN02_HEADER_SIZE			(sizeof(::Barry::Protocol::JVMUnknown02))

struct JVMPacket
{
	uint16_t	socket;
	uint16_t	size;		// total size of data packet

	union PacketData
	{
		JVMCommand		command;
		JVMResponse		response;
		JVMModulesList		moduleslist;
		JVMThreadsList		threadslist;
		JVMUnknown01		unknown01;
		JVMUnknown02		unknown02;
		uint32_t		address;
		uint16_t		expect;
		uint16_t		msglength;
		uint16_t		value;
		uint8_t			status;
		uint8_t			raw[1];
	} __attribute__ ((packed)) u;

} __attribute__ ((packed));
#define SB_JVMPACKET_HEADER_SIZE		(sizeof(::Barry::Protocol::JVMPacket) - sizeof(::Barry::Protocol::JVMPacket::PacketData))


/////////////////////////////////////////////////////////////////////////////
// Raw channel packet structure
struct ChannelPacket
{
	uint16_t	socket;		// socket ID... 0 exists by default
	uint16_t	size;		// total size of data packet
	
	union PacketData
	{
		uint8_t			data[1];
	} __attribute__ ((packed)) u;
} __attribute__ ((packed));
#define SB_CHANNELPACKET_HEADER_SIZE		(sizeof(::Barry::Protocol::ChannelPacket) - sizeof(::Barry::Protocol::ChannelPacket::PacketData))

#define SB_CHANNELPACKET_MAX_DATA_SIZE		0x3FFC

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
#define SB_PACKET_SOCKET_SIZE			(sizeof(uint16_t)) // size needed to read the socket in a packet
#define SB_PACKET_HEADER_SIZE			(sizeof(::Barry::Protocol::Packet) - sizeof(::Barry::Protocol::Packet::PacketData))

// WARNING : For JavaLoader we have some packet with 5 size !
#define MIN_PACKET_SIZE			5
#define MIN_PACKET_DATA_SIZE		4


// maximum sizes
#define MAX_PACKET_SIZE			0x400	// anything beyond this needs to be fragmented
#define MAX_PACKET_DATA_SIZE		0x7FC	// for data packet (JavaLoader)

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

#define SB_SEQUENCE_PACKET_SIZE			(SB_PACKET_HEADER_SIZE + sizeof(::Barry::Protocol::SequenceCommand))
#define SB_SOCKET_PACKET_HEADER_SIZE		(SB_PACKET_HEADER_SIZE + SOCKET_COMMAND_HEADER_SIZE)
#define SB_MODE_PACKET_COMMAND_SIZE		(SB_SOCKET_PACKET_HEADER_SIZE + sizeof(::Barry::Protocol::ModeSelect) - sizeof(::Barry::Protocol::ModeSelect::ResponseBlock))
#define SB_MODE_PACKET_RESPONSE_SIZE		(SB_SOCKET_PACKET_HEADER_SIZE + sizeof(::Barry::Protocol::ModeSelect))


// Macros
#define COMMAND(data)				(((const ::Barry::Protocol::Packet *)data.GetData())->command)
#define IS_COMMAND(data, cmd)			(COMMAND(data) == cmd)
#define MAKE_PACKET(var, data)			const ::Barry::Protocol::Packet *var = (const ::Barry::Protocol::Packet *) (data).GetData()
#define MAKE_JLPACKET(var, data)		const ::Barry::Protocol::JLPacket *var = (const ::Barry::Protocol::JLPacket *) (data).GetData()
#define MAKE_JVMPACKET(var, data)		const ::Barry::Protocol::JVMPacket *var = (const ::Barry::Protocol::JVMPacket *) (data).GetData()
#define MAKE_CHANNELPACKET(var, data)		const ::Barry::Protocol::ChannelPacket *var = (const ::Barry::Protocol::ChannelPacket *) (data).GetData()
#define MAKE_PACKETPTR_BUF(var, ptr)		::Barry::Protocol::Packet *var = (::Barry::Protocol::Packet *)ptr
#define MAKE_JLPACKETPTR_BUF(var, ptr)		::Barry::Protocol::JLPacket *var = (::Barry::Protocol::JLPacket *)ptr
#define MAKE_JVMPACKETPTR_BUF(var, ptr)		::Barry::Protocol::JVMPacket *var = (::Barry::Protocol::JVMPacket *)ptr
#define MAKE_CHANNELPACKETPTR_BUF(var, ptr)	::Barry::Protocol::ChannelPacket *var = (::Barry::Protocol::ChannelPacket *)ptr
#define MAKE_RECORD(type,var,data,off)		type *var = (type *) ((data).GetData() + (off))
#define MAKE_RECORD_PTR(type,var,data,off)	type *var = (type *) ((data) + (off))

// fragmentation protocol
// send DATA first, then keep sending DATA packets, FRAGMENTing
// as required until finished, then send DONE.  Both sides behave
// this way, so different sized data can be sent in both
// directions
//
// the fragmented piece only has a the param header, and then continues
// right on with the data



// checks packet size and throws BError if not right
void CheckSize(const Barry::Data &packet, size_t requiredsize);
unsigned int GetSize(const Barry::Data &packet);

}} // namespace Barry::Protocol

#endif

