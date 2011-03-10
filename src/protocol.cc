///
/// \file	protocol.cc
///		USB Blackberry bulk protocol API
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "endian.h"
#include "error.h"
#include "debug.h"

#include <sstream>

namespace Barry { namespace Protocol {

// This function is only valid for Packet, JLPacket, and JVMPacket structs,
// as long as they don't differ from each other in header layout, when
// it comes to the .size field.  (see protostructs.h)
void CheckSize(const Data &packet, size_t requiredsize)
{
	const Packet *p = (const Packet *) packet.GetData();

	// when packets are larger than 0xFFFF bytes, packet->size is no
	// longer reliable, so we go with the Data class size
	if( (packet.GetSize() >= 4 && btohs(p->size) != packet.GetSize() && packet.GetSize() <= 0xFFFF) ||
	    packet.GetSize() < requiredsize )

	{
		BadSize bs(packet.GetSize() >= 4 ? btohs(p->size) : 0,
			packet.GetSize(), requiredsize);
		eout(bs.what());
		eout(packet);
		throw bs;
	}
}

unsigned int GetSize(const Data &packet)
{
	CheckSize(packet, 4);

	// when packets are larger than 0xFFFF bytes, packet->size is no
	// longer reliable, so we go with the Data class size
	if( packet.GetSize() > 0xFFFF ) {
		return packet.GetSize();
	}
	else {
		const Packet *p = (const Packet *) packet.GetData();
		return btohs(p->size);
	}
}

bool IsSequencePacket(const Barry::Data &data)
{
	if( data.GetSize() == SB_SEQUENCE_PACKET_SIZE ) {
		MAKE_PACKET(rpack, data);
		if( rpack->socket == 0 &&
		    rpack->command == SB_COMMAND_SEQUENCE_HANDSHAKE )
		{
			return true;
		}
	}
	return false;	// not a sequence packet
}

}} // namespace Barry::Protocol

