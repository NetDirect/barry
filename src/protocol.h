///
/// \file	protocol.h
///		USB Blackberry bulk protocol API
///

#ifndef __SYNCBERRY_PROTOCOL_H__
#define __SYNCBERRY_PROTOCOL_H__

#include <stdint.h>

namespace Syncberry {


struct SocketCommand
{
	uint16_t	socket;
	uint8_t		param;
};

struct SequenceCommand
{
	uint8_t		unknown1;
	uint8_t		unknown2;
	uint8_t		unknown3;
	uint32_t	sequenceId;
};

struct DBCommand
{
	uint8_t		command;	// see below
	uint16_t	database_id;	// value from the Database Database
	uint8_t		data[1];
};

struct DBResponse
{
	uint8_t		command;
	uint8_t		data[1];
};

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
				SocketCommand	socket;
				SequenceCommand	sequence;
				uint8_t		raw[1];
			} data;
		} simple;


		// and some commands do, even in their fragmented packets
		struct ParamPacket
		{
			uint8_t		param;
			union ParamPacketData
			{
				DBCommand	db;
				DBResponse	db_r;
				uint8_t		raw[1];
			} data;
		} param;

	} data;
};

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



// sockets seen
#define SB_SOCKET_INIT		0x07
#define SB_SOCKET_COMM		0x06
//#define SB_SOCKET_???		0x03	// unknown

// socket params
#define SB_SOCKET_INIT_PARAM	0x06


// packet commands (Packet.command: has response codes too)
#define SB_COMMAND_OPEN_SOCKET		0x0a
#define SB_COMMAND_CLOSE_SOCKET		0x0b
#define SB_COMMAND_CLOSED_SOCKET	0x0c	// response #2
#define SB_COMMAND_SEQUENCE_HANDSHAKE	0x13
#define SB_COMMAND_DB_DATA		0x40
#define SB_COMMAND_DB_FRAGMENTED	0x60
#define SB_COMMAND_DB_DONE		0x41


// DB Operation Command
#define SB_DBOP_GET_DBDB		0x4a
#define SB_DBOP_GET_COUNT		0x4e
#define SB_DBOP_GET_RECORDS		0x4f


// fragmentation protocol
// send DATA first, then keep sending DATA packets, FRAGMENTing
// as required until finished, then send DONE.  Both sides behave
// this way, so different sized data can be sent in both
// directions
//
// the fragmented piece only has a the param header, and then continues
// right on with the data

} // namespace Syncberry

#endif

