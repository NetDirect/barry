///
/// \file	m_javaloader.h
///		Mode class for the JavaLoader mode
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
	Copyright (C) 2008-2009, Nicolas VIVIEN

	Some parts are inspired from m_desktop.h

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

#ifndef __BARRY_M_JAVALOADER_H__
#define __BARRY_M_JAVALOADER_H__

#include "dll.h"
#include "socket.h"
#include "record.h"

namespace Barry {

// forward declarations
class Parser;
class Builder;
class Controller;

namespace Mode {

//
// Desktop class
//
/// The main interface class to the device databases.
///
/// To use this class, use the following steps:
///
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::JavaLoader object, passing in the Controller
///		object during construction
///	- Call Open() to open database socket and finish constructing.
///	- Call LoadDatabase() to retrieve and store a database
///
class BXEXPORT JavaLoader
{
public:
	enum CommandType { Unknown, DatabaseAccess };

private:
	Controller &m_con;

	SocketHandle m_socket;

	CommandTable m_commandTable;

	uint16_t m_ModeSocket;			// socket recommended by device
						// when mode was selected

protected:

public:
	JavaLoader(Controller &con);
	~JavaLoader();

	//////////////////////////////////
	// primary operations - required before anything else

	void Open(const char *password = 0);
	void RetryPassword(const char *password);

	//////////////////////////////////
	// API
	void StartStream();
	void SendStream(char *buffer, int size);
	void StopStream(void);
};

}} // namespace Barry::Mode

#endif

