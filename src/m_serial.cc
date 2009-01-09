///
/// \file	m_serial.cc
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

#include "m_serial.h"
#include "controller.h"
#include "protostructs.h"
#include "endian.h"
#include "debug.h"
#include <stdexcept>

namespace Barry { namespace Mode {

//////////////////////////////////////////////////////////////////////////////
// Mode::Serial class

Serial::Serial(	Controller &con,
		DeviceDataCallback callback,
		void *callback_context)
	: m_con(con)
	, m_ModeSocket(0)
	, m_CtrlSocket(0)
	, m_callback(callback)
	, m_callback_context(callback_context)
{
	if( !m_con.HasQueue() )
		throw std::logic_error("A SocketRoutingQueue is required in the Controller class when using Mode::Serial.");
}

Serial::~Serial()
{
}


//////////////////////////////////////////////////////////////////////////////
// protected API / static functions

void Serial::DataCallback(void *context, Data *data)
{
	ddout("Serial::DataCallback called");

	Serial *ser = (Serial*) context;

	if( data->GetSize() <= 4 )
		return;	// nothing to do

	// call callback if available
	if( ser->m_callback ) {
		(*ser->m_callback)(ser->m_callback_context,
			data->GetData() + 4,
			data->GetSize() - 4);
	}
//	else {
//		// append data to readCache
//		FIXME;
//	}
}

void Serial::CtrlCallback(void *context, Data *data)
{
//	Serial *ser = (Serial*) context;

	// just dump to stdout, and do nothing
	ddout("CtrlCallback received:\n" << *data);
}

//////////////////////////////////////////////////////////////////////////////
// public API

void Serial::Open(const char *password)
{
	if( m_ModeSocket ) {
		m_data->Close();
		m_data.reset();
		m_ModeSocket = 0;
	}

	if( m_CtrlSocket ) {
		m_ctrl->Close();
		m_ctrl.reset();
		m_CtrlSocket = 0;
	}

	m_ModeSocket = m_con.SelectMode(Controller::UsbSerData);
	m_data = m_con.m_zero.Open(m_ModeSocket, password);

	m_CtrlSocket = m_con.SelectMode(Controller::UsbSerCtrl);
	m_ctrl = m_con.m_zero.Open(m_CtrlSocket, password);

	// register callback for incoming data, for speed
	m_data->RegisterInterest(DataCallback, this);
	m_ctrl->RegisterInterest(CtrlCallback, this);

	const unsigned char start[] =
		{ 0, 0, 0x0a, 0, 0x01, 0x01, 0xc2, 0x00, 0x40, 0x00 };
	Data block(start, sizeof(start));
	m_ctrl->Send(block);
}

void Serial::Close()
{
	ddout("Serial:: Closing connection.");
}

/*
// FIXME - if this behaviour is truly common between modes, create
// a common base class for this.
void Serial::RetryPassword(const char *password)
{
	if( m_data.get() || m_ctrl.get() )
		throw std::logic_error("Socket already open in Serial::RetryPassword");

	m_data = m_con.m_zero.OpenDBSocket(m_ModeSocket, password);
	m_ctrl = m_con.m_zero.OpenDBSocket(m_CtrlSocket, password);

	// register callback for incoming data, for speed
	m_data->RegisterInterest(DataCallback, this);
}
*/

/*
// can be called from separate thread
void Serial::SerialRead(Data &data, int timeout)
{
	m_socket.Receive(data, timeout);
}
*/

void Serial::Write(const Data &data, int timeout)
{
	if( data.GetSize() <= 0 )
		return;	// nothing to do

	if( !m_data.get() )
		throw std::logic_error("Must call Open() before Write() in Mode::Serial");

	// filter data for PPP, and prepend 4 bytes
	Data &filtered = m_filter.Write(data, 4);

	// setup header (only size needed, as socket will be set by socket class)
	unsigned char *buf = filtered.GetBuffer();
	MAKE_PACKETPTR_BUF(spack, buf);
	spack->size = htobs(filtered.GetSize());

	// send via appropriate socket
	m_data->Send(filtered, timeout);
}

}} // namespace Barry::Mode

