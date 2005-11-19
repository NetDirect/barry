///
/// \file	protocol.cc
///		USB Blackberry bulk protocol API
///

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

