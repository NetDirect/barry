///
/// \file	protocol.h
///		USB Blackberry bulk protocol API
///

#ifndef __SYNCBERRY_PROTOCOL_H__
#define __SYNCBERRY_PROTOCOL_H__

#include <stdint.h>

// forward declarations
class Data;

namespace Syncberry {


struct SocketCommand
{
	uint16_t	socket;
	uint8_t		param;
} __attribute__ ((packed));

struct SequenceCommand
{
	uint8_t		unknown1;
	uint8_t		unknown2;
	uint8_t		unknown3;
	uint32_t	sequenceId;
} __attribute__ ((packed));

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

struct DBCommand
{
	uint8_t		command;	// see below
	uint16_t	databaseId;	// value from the Database Database
	uint8_t		data[1];
} __attribute__ ((packed));

struct DBResponse
{
	uint8_t		command;
	uint32_t	unknown;
	uint16_t	sequenceCount;
	uint8_t		data[1];
} __attribute__ ((packed));

struct Packet
{
	uint16_t	socket;		// socket ID... 0 is always there
	uint16_t	size;		// total size of data packet
	uint8_t		command;

	union PacketData
	{
		// some commands have no parameter
		struct SimplePacket
		{
			union SimplePacketData
			{
				SocketCommand		socket;
				SequenceCommand		sequence;
				ModeSelectCommand	mode;
				uint8_t		raw[1];
			} __attribute__ ((packed)) data;
		}  __attribute__ ((packed)) simple;


		// and some commands do, even in their fragmented packets
		struct ParamPacket
		{
			uint8_t		param;
			union ParamPacketData
			{
				DBCommand		db;
				DBResponse		db_r;
				uint8_t			raw[1];
			} __attribute__ ((packed)) data;
		}  __attribute__ ((packed)) param;

	} __attribute__ ((packed)) data;
} __attribute__ ((packed));

// minimum required sizes for various responses
#define MIN_PACKET_SIZE		6


// maximum sizes
#define MAX_PACKET_SIZE		0x400	// anything beyond this needs to be
					// fragmented
// various useful sizes
// frag header is the header of a normal packet, plus the param
#define SB_PACKET_HEADER_SIZE	(sizeof(Syncberry::Packet) - sizeof(Syncberry::Packet::PacketData))
#define SB_PARAM_HEADER_SIZE	(sizeof(Syncberry::Packet::PacketData::ParamPacket) - sizeof(Syncberry::Packet::PacketData::ParamPacket::ParamPacketData))
#define SB_FRAG_HEADER_SIZE	(SB_PACKET_HEADER_SIZE + SB_PARAM_HEADER_SIZE)
#define SB_SEQUENCE_PACKET_SIZE	(SB_PACKET_HEADER_SIZE + sizeof(Syncberry::SequenceCommand))
#define SB_SOCKET_PACKET_SIZE	(SB_PACKET_HEADER_SIZE + sizeof(Syncberry::SocketCommand))
#define SB_MODE_PACKET_COMMAND_SIZE	(SB_PACKET_HEADER_SIZE + sizeof(Syncberry::ModeSelectCommand) - sizeof(Syncberry::ModeSelectCommand::ResponseBlock))
#define SB_MODE_PACKET_RESPONSE_SIZE	(SB_PACKET_HEADER_SIZE + sizeof(Syncberry::ModeSelectCommand))



// packet commands (Packet.command: has response codes too)
#define SB_COMMAND_SELECT_MODE		0x07
#define SB_COMMAND_MODE_SELECTED	0x08
#define SB_COMMAND_OPEN_SOCKET		0x0a
#define SB_COMMAND_CLOSE_SOCKET		0x0b
#define SB_COMMAND_CLOSED_SOCKET	0x0c
#define SB_COMMAND_OPENED_SOCKET	0x10
#define SB_COMMAND_SEQUENCE_HANDSHAKE	0x13
#define SB_COMMAND_DB_DATA		0x40
#define SB_COMMAND_DB_FRAGMENTED	0x60
#define SB_COMMAND_DB_DONE		0x41


// mode constants
#define SB_MODE_REQUEST_SOCKET		0x00ff


// param command parameters
//#define SB_PARAM_DEFAULT		0xff


// DB Operation Command
#define SB_DBOP_GET_DBDB		0x4a
#define SB_DBOP_OLD_GET_DBDB		0x4c
#define SB_DBOP_GET_COUNT		0x4e
#define SB_DBOP_GET_RECORDS		0x4f


// Macros
#define COMMAND(data)			(((const Syncberry::Packet *)data.GetData())->command)
#define IS_COMMAND(data, cmd)		(COMMAND(data) == cmd)
#define MAKE_PACKET(var, data)		const Syncberry::Packet *var = (const Syncberry::Packet *) data.GetData()

// fragmentation protocol
// send DATA first, then keep sending DATA packets, FRAGMENTing
// as required until finished, then send DONE.  Both sides behave
// this way, so different sized data can be sent in both
// directions
//
// the fragmented piece only has a the param header, and then continues
// right on with the data



// checks packet size and throws SBError if not right
void CheckSize(const Data &packet, int requiredsize = MIN_PACKET_SIZE);

} // namespace Syncberry

#endif

