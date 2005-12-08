///
/// \file	parser.cc
///		Virtual parser functor class.  Also, all protocol-specific
///		parser code goes in here.
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

#include "parser.h"
#include "protocol.h"
#include "protostructs.h"

namespace Barry {

bool Parser::GetOperation(const Data &data, unsigned int &operation)
{
	// check size to make sure we have up to the DBAccess operation byte
	if( (unsigned int)data.GetSize() < (SB_PACKET_DBACCESS_HEADER_SIZE + 1) )
		return false;

	MAKE_PACKET(pack, data);
	operation = pack->data.db.data.db.operation;
	return true;
}

size_t Parser::GetHeaderSize(size_t recordsize) const
{
	// calculate the full header size, which (for records) is a DBACCESS
	// header size, plus the header size of recordsize...
	return SB_PACKET_DBACCESS_HEADER_SIZE + recordsize - sizeof(Barry::CommonField);
}

} // namespace Barry

