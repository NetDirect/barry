///
/// \file	controller.h
///		High level BlackBerry API class
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

#ifndef __BARRY_CONTROLLER_H__
#define __BARRY_CONTROLLER_H__

#include "usbwrap.h"
#include "probe.h"
#include "socket.h"
#include "record.h"

namespace Barry {

// forward declarations
class Parser;

class Controller
{
public:
	enum ModeType { Unspecified, Bypass, Desktop, JavaLoader };
	enum CommandType { Unknown, DatabaseAccess };

private:
	Usb::Device m_dev;
	Usb::Interface m_iface;
	uint32_t m_pin;

	Socket m_socket;

	CommandTable m_commandTable;
	DatabaseDatabase m_dbdb;

	ModeType m_mode;

protected:
	void SelectMode(ModeType mode, uint16_t &socket, uint8_t &flag);
	unsigned int GetCommand(CommandType ct);

	void LoadCommandTable();
	void LoadDBDB();

public:
	Controller(const ProbeResult &device);
	~Controller();

	// meta access
	const DatabaseDatabase& GetDBDB() const { return m_dbdb; }
	unsigned int GetDBID(const std::string &name) const;

	// general operations
	void OpenMode(ModeType mode);

	// database-specific
	void LoadDatabase(unsigned int dbId, Parser &parser);
};

} // namespace Barry

#endif

