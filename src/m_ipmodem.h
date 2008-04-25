///
/// \file	m_ipmodem.h
///		Mode class for GPRS modem mode (using endpoints on
///		modern devices)
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

#ifndef __BARRY_M_IPMODEM_H__
#define __BARRY_M_IPMODEM_H__

#include "dll.h"
#include "usbwrap.h"
#include "data.h"
#include <pthread.h>

namespace Barry {

// forward declarations
class Controller;

namespace Mode {

class BXEXPORT IpModem
{
public:
	typedef void (*DeviceDataCallback)(void *context, const unsigned char *data, int len);

private:
	Controller &m_con;
	Usb::Device &m_dev;

	Data m_writeBuf;			// used for 0x7e handling

	// thread data
	volatile bool m_continue_reading;
	pthread_t m_modem_read_thread;

	// write flags
	bool m_ppp_mode;
	unsigned char m_last;

	// external callbacks
	DeviceDataCallback m_callback;
	void *m_callback_context;

protected:
	static void *DataReadThread(void *userptr);

public:
	IpModem(Controller &con, DeviceDataCallback callback, void *callback_context);
	~IpModem();

	//////////////////////////////////
	// general operations
	void Open();

	//////////////////////////////////
	// UsbSerData mode - modem specific

//	void Read(Data &data, int timeout); // can be called from separate thread
	void Write(const Data &data, int timeout = -1);
};

}} // namespace Barry::Mode

#endif

