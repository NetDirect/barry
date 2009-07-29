///
/// \file	protocol.h
///		USB Blackberry bulk protocol API constants
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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
#define SB_COMMAND_ECHO			0x01
#define SB_COMMAND_ECHO_REPLY		0x02
#define SB_COMMAND_RESET		0x03
#define SB_COMMAND_RESET_REPLY		0x04
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

// JavaLoader commands
#define SB_COMMAND_JL_HELLO		0x64	// This could be a general ACK in both directions
#define SB_COMMAND_JL_HELLO_ACK		0x65	// From device after host HELLO
#define SB_COMMAND_JL_GOODBYE		0x8d
#define SB_COMMAND_JL_SET_UNKNOWN1	0x70	// Initial sequence, 0
#define SB_COMMAND_JL_SET_COD_FILENAME	0x80
#define SB_COMMAND_JL_SET_COD_SIZE	0x67	// Always big endian
#define SB_COMMAND_JL_SEND_DATA		0x68
#define SB_COMMAND_JL_SET_TIME		0x7c
#define SB_COMMAND_JL_GET_SCREENSHOT	0x87
#define SB_COMMAND_JL_DEVICE_INFO	0x71
#define SB_COMMAND_JL_OS_METRICS	0x78
#define SB_COMMAND_JL_BOOTROM_METRICS	0x79
#define SB_COMMAND_JL_GET_DIRECTORY	0x6d
#define SB_COMMAND_JL_GET_DATA_ENTRY	0x6e	// Used for both DIR and SCREENSHOT
#define SB_COMMAND_JL_GET_SUBDIR	0x7f
#define SB_COMMAND_JL_GET_SUBDIR_ENTRY	0x7d
#define SB_COMMAND_JL_ERASE		0x69
#define SB_COMMAND_JL_FORCE_ERASE	0x7b
#define SB_COMMAND_JL_UNKNOWN3		0x63
#define SB_COMMAND_JL_GET_LOG		0x73
#define SB_COMMAND_JL_GET_LOG_ENTRY	0x74
#define SB_COMMAND_JL_CLEAR_LOG		0x88
#define SB_COMMAND_JL_SAVE_MODULE	0x7e
#define SB_COMMAND_JL_WIPE_APPS		0x6a
#define SB_COMMAND_JL_WIPE_FS		0x6b
#define SB_COMMAND_JL_LOG_STRACES	0x8e
#define SB_COMMAND_JL_RESET_FACTORY	0x91

// JavaLoader response
#define SB_COMMAND_JL_ACK		0x64
#define SB_COMMAND_JL_READY		0x01
#define SB_COMMAND_JL_RESET_REQUIRED	0x78	// Occurs after GOODBYE when handheld reset is required
#define SB_COMMAND_JL_COD_IN_USE	0x6c	// Perhaps "BUSY" is also a good name?
#define SB_COMMAND_JL_COD_NOT_FOUND	0x69
#define SB_COMMAND_JL_NOT_SUPPORTED	0x71	// Occurs when device does not support a command

// JavaLoader data
#define SB_DATA_JL_SUCCESS		0x64	// Device has accepted the data packet
#define SB_DATA_JL_INVALID		0x68	// Device returns this code if the application isn't valid.


// JavaDebug commands
#define SB_COMMAND_JD_UNKNOWN01		0x53
#define SB_COMMAND_JD_UNKNOWN02		0x01
#define SB_COMMAND_JD_UNKNOWN03		0x6f
#define SB_COMMAND_JD_UNKNOWN04		0x8a
#define SB_COMMAND_JD_UNKNOWN05		0x90
#define SB_COMMAND_JD_UNKNOWN06		0x44
#define SB_COMMAND_JD_UNKNOWN07		0x45
#define SB_COMMAND_JD_UNKNOWN08		0x54
#define SB_COMMAND_JD_UNKNOWN09		0x33
#define SB_COMMAND_JD_UNKNOWN10		0x46
#define SB_COMMAND_JD_GET_MODULES_LIST	0x8d	// Get all Java modules list with their address and ID
#define SB_COMMAND_JD_GET_CONSOLE_MSG	0x40	// Get console message
#define SB_COMMAND_JD_GO		0x02	// Go
#define SB_COMMAND_JD_GET_STATUS		0x06	// Get status
#define SB_COMMAND_JD_SET_BREAKPOINT	0x21	// Set breakpoint
#define SB_COMMAND_JD_RM_BREAKPOINT	0x22	// Remove breakpoint

// JavaDebug response
#define SB_COMMAND_JD_GET_DATA_ENTRY	0x06


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

