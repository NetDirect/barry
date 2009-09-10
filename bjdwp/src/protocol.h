///
/// \file	protocol.h
///		Low level USB protocol constants and structs
///

/*
    Copyright (C) 2009, Nicolas VIVIEN

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

#ifndef __BARRYJDWP_PROTOCOL_H__
#define __BARRYJDWP_PROTOCOL_H__


#include <stdint.h>
#include <sys/types.h>


// Command set list
#define JDWP_CMDSET_VIRTUALMACHINE			1
#define	JDWP_CMDSET_REFERECENTYPE			2
#define	JDWP_CMDSET_CLASSTYPE				3
#define JDWP_CMDSET_ARRAYTYPE				4
#define JDWP_CMDSET_INTERFACETYPE			5
#define JDWP_CMDSET_METHOD					6
#define JDWP_CMDSET_FIELD					8
#define JDWP_CMDSET_OBJECTREFERENCE			9
#define JDWP_CMDSET_STRINGREFERENCE			10
#define JDWP_CMDSET_THREADREFERENCE			11
#define JDWP_CMDSET_THREADGROUPREFERENCE	12
#define JDWP_CMDSET_ARRAYREFERENCE			13
#define JDWP_CMDSET_CLASSLOADERREFERENCE	14
#define JDWP_CMDSET_EVENTREQUEST			15
#define JDWP_CMDSET_STACKFRAME				16
#define JDWP_CMDSET_CLASSOBJECTREFERENCE	17
#define JDWP_CMDSET_EVENT					64

// Command list - VirtualMachine
#define JDWP_CMD_VERSION					1
#define JDWP_CMD_ALLCLASSES					3
#define JDWP_CMD_ALLTHREADS					4
#define JDWP_CMD_DISPOSE					6
#define JDWP_CMD_IDSIZES					7
#define JDWP_CMD_SUSPEND					8
#define JDWP_CMD_RESUME						9
#define JDWP_CMD_CLASSPATHS					13

// Command list - EventRequest
#define JDWP_CMD_SET						1


namespace JDWP { namespace Protocol {

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

#define JDWP_COMMAND_HEADER_SIZE			(sizeof(Protocol::PacketCommand))


// Packet response
//-----------------

struct PacketVirtualMachineIDSizes {
	uint32_t fieldIDSize;
	uint32_t methodIDSize;
	uint32_t objectIDSize;
	uint32_t referenceTypeIDSize;
	uint32_t frameIDSize;
} __attribute__ ((packed));

#define JDWP_PACKETVIRTUALMACHINEIDSIZES_DATA_SIZE		sizeof(Protocol::PacketVirtualMachineIDSizes)


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

#define JDWP_RESPONSE_HEADER_SIZE			(sizeof(Protocol::PacketResponse) - sizeof(Protocol::PacketResponse::PacketResponseData))


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

#define JDWP_PACKET_HEADER_SIZE			(sizeof(Protocol::Packet) - sizeof(Protocol::Packet::PacketType))


#define MAKE_JDWPPACKET(var, data)		const Protocol::Packet *var = (const Protocol::Packet *) (data).GetData()
#define MAKE_JDWPPACKETPTR_BUF(var, ptr)		Protocol::Packet *var = (Protocol::Packet *)ptr


}} // namespace JDWP::Protocol

#endif

