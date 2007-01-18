///
/// \file	packet.h
///		Low level protocol packet builder class.
///		Has knowledge of specific protocol commands in order
///		to hide protocol details behind an API.
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

#ifndef __BARRY_PACKET_H__
#define __BARRY_PACKET_H__

#include <stdint.h>

namespace Barry { class Data; }

namespace Barry {

// forward declarations
class Parser;
class Builder;
class Socket;
class Controller;

//
// Packet class
//
/// Provides an API for building and analyzing raw protocol packets.
/// This class relies on 3 external objects: a Controller object,
/// a send Data buffer, and a receive data buffer.  Socket and
/// connection details are retrieved on a readonly basis from the
/// Controller object, but both send and receive buffers can be
/// modified.
///
/// Note that the receive buffer may be modified
/// during a packet send, and this Packet class provides API helpers
/// to analyze the results.
///
class Packet
{
	friend class Socket;

private:
	Controller &m_con;
	Data &m_send, &m_receive;
	unsigned int m_last_dbop;	// last database operation

protected:

public:
	Packet(Controller &con, Data &send, Data &receive);
	~Packet();

	//////////////////////////////////
	// meta access

	//////////////////////////////////
	// packet building

	// commands that correspond to the DB operation
	// constants in protocol.h
	void ClearDatabase(unsigned int dbId);
	void GetDBDB();
	void GetRecordStateTable(unsigned int dbId);
	void SetRecordFlags(unsigned int dbId, unsigned int stateTableIndex, uint8_t flag1);
	void DeleteRecordByIndex(unsigned int dbId, unsigned int stateTableIndex);
	void GetRecordByIndex(unsigned int dbId, unsigned int stateTableIndex);
	bool SetRecordByIndex(unsigned int dbId, unsigned int stateTableIndex, Builder &build);
	void GetRecords(unsigned int dbId);
	bool SetRecord(unsigned int dbId, Builder &build);


	//////////////////////////////////
	// response analysis

	unsigned int Command() const;	// throws BError if receive isn't big enough
	unsigned int ReturnCode() const;	// throws FIXME if packet doesn't support it
	unsigned int DBOperation() const; // throws BError on size trouble

	bool Parse(Parser &parser);	// switches based on last m_send command

	// response parsers
};

} // namespace Barry

#endif

