///
/// \file	m_serial.h
///		Mode class for serial / GPRS modem mode
///

/*
    Copyright (C) 2008-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "dll.h"
#include "modem.h"
#include "socket.h"
#include "data.h"
#include "pppfilter.h"

namespace Barry {

// forward declarations
class Controller;

namespace Mode {

class BXEXPORT Serial : public Modem
{
public:
	typedef void (*DeviceDataCallback)(void *context, const unsigned char *data, int len);

private:
	Controller &m_con;

	SocketHandle m_data;
	SocketHandle m_ctrl;

	uint16_t m_ModeSocket;			// socket recommended by device
						// when mode was selected
	uint16_t m_CtrlSocket;

	// PPP filtering
	PppFilter m_filter;

	// UsbSerData cache
//	Data m_readCache;

	// external callbacks
	DeviceDataCallback m_callback;
	void *m_callback_context;

protected:
	static void DataCallback(void *context, Data *data);
	static void CtrlCallback(void *context, Data *data);

public:
	Serial(Controller &con, DeviceDataCallback callback, void *callback_context);
	~Serial();

	//////////////////////////////////
	// general operations
	void Open(const char *password = 0);	// FIXME password needed?  if not,
						// then we can remove it from
						// the Modem base class
	void Close();
	void RetryPassword(const char *password);

	//////////////////////////////////
	// UsbSerData mode - modem specific

//	void Read(Data &data, int timeout); // can be called from separate thread
	void Write(const Data &data, int timeout = -1);
};

}} // namespace Barry::Mode

#endif

