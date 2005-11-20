///
/// \file	protocol.cc
///		USB Blackberry bulk protocol API
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "data.h"
#include "error.h"
#include "debug.h"

#include <sstream>

namespace Barry {

void CheckSize(const Data &packet, int requiredsize)
{
	const Packet *p = (const Packet *) packet.GetData();
	if( p->size != (unsigned int) packet.GetSize() ||
	    packet.GetSize() < requiredsize )
	{
		std::ostringstream oss;
		oss << "Bad packet size. Packet: " << p->size
		    << ". DataSize(): " << packet.GetSize()
		    << ". Required size: " << requiredsize;
		eout(oss.str());
		eout(packet);
		throw SBError(oss.str());
	}
}

} // namespace Barry

