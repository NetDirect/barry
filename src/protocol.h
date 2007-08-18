///
/// \file	protocol.h
///		USB Blackberry bulk protocol API constants
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

#ifndef __BARRY_PROTOCOL_H__
#define __BARRY_PROTOCOL_H__

// packet commands (Packet.command: has response codes too)
#define SB_COMMAND_FETCH_ATTRIBUTE	0x05
#define SB_COMMAND_FETCHED_ATTRIBUTE	0x06
#define SB_COMMAND_SELECT_MODE		0x07
#define SB_COMMAND_MODE_SELECTED	0x08
#define SB_COMMAND_OPEN_SOCKET		0x0a
#define SB_COMMAND_CLOSE_SOCKET		0x0b
#define SB_COMMAND_CLOSED_SOCKET	0x0c
#define SB_COMMAND_PASSWORD_CHALLENGE	0x0e
#define SB_COMMAND_PASSWORD		0x0f
#define SB_COMMAND_OPENED_SOCKET	0x10
#define SB_COMMAND_PASSWORD_FAILED	0x11
#define SB_COMMAND_SEQUENCE_HANDSHAKE	0x13
#define SB_COMMAND_DB_DATA		0x40
#define SB_COMMAND_DB_FRAGMENTED	0x60
#define SB_COMMAND_DB_DONE		0x41


// mode constants
#define SB_MODE_REQUEST_SOCKET		0x00ff


// object and attribute ID codes (for ZeroPacket::GetAttribute())
// meanings for most of these are unknown
#define SB_OBJECT_INITIAL_UNKNOWN		0x14
#define		SB_ATTR_INITIAL_UNKNOWN		0x01
#define SB_OBJECT_PROFILE			0x08
#define		SB_ATTR_PROFILE_DESC		0x02
#define		SB_ATTR_PROFILE_PIN		0x04
#define SB_OBJECT_SOCKET_UNKNOWN		0x04


// param command parameters
//#define SB_PARAM_DEFAULT		0xff


// DB Operation Command
#define SB_DBOP_SET_RECORD		0x41
#define SB_DBOP_CLEAR_DATABASE		0x43
#define SB_DBOP_GET_DBDB		0x4a
#define SB_DBOP_OLD_GET_DBDB		0x4c
#define SB_DBOP_GET_COUNT		0x4e
#define SB_DBOP_GET_RECORDS		0x4f
#define SB_DBOP_OLD_GET_RECORDS		0x42
#define SB_DBOP_OLD_GET_RECORDS_REPLY	0x44

#define SB_DBOP_GET_RECORD_STATE_TABLE	0x53	// replies with 0x60, 0x41
#define SB_DBOP_SET_RECORD_FLAGS	0x54	// replies with 0x41
						// used to clear dirty flag
#define SB_DBOP_GET_RECORD_BY_INDEX	0x46	// replies with 0x44
	// John's device uses 0x50, with a reply of 0x4f (!)
	// Then uses 0x55 as usual
	// Delete uses 0x50/0x4f to check as well, then uses 0x52 as
	// usual to do the actual delete.
#define SB_DBOP_SET_RECORD_BY_INDEX	0x55	// replies with 0x41
#define SB_DBOP_DELETE_RECORD_BY_INDEX	0x52	// intellisync does a GET(0x46)
						// first, probably to make sure
						// it has the right data and
						// record

#endif

