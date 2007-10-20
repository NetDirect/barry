///
/// \file	m_serial.h
///		Mode class for serial / GPRS modem mode
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

#ifndef __BARRY_M_SERIAL_H__
#define __BARRY_M_SERIAL_H__

#include "usbwrap.h"
#include "probe.h"
#include "socket.h"
#include "record.h"
#include "data.h"

namespace Barry {

namespace Mode {

class Serial
{
//	friend class Barry::DBPacket;

private:
	Controller &m_con;

	SocketHandle m_socket;

	// UsbSerData cache
	Data m_writeCache, m_readCache;

	// tracking of open socket, and the need to reset
//	bool m_halfOpen;

protected:

public:
	Serial(Controller &con);
	~Serial();

	//////////////////////////////////
	// general operations
	void Open(const char *password = 0);	// FIXME password needed?
	void RetryPassword(const char *password);

	//////////////////////////////////
	// UsbSerData mode - modem specific

	void SerialRead(Data &data, int timeout); // can be called from separate thread
	void SerialWrite(const Data &data);
};

} // namespace Barry

#endif

