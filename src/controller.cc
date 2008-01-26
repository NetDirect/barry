///
/// \file	controller.cc
///		High level Barry API class
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

#include "controller.h"
//#include "common.h"
//#include "protocol.h"
//#include "protostructs.h"
//#include "error.h"
//#include "data.h"
//#include "parser.h"
//#include "builder.h"
//#include "endian.h"
//#include "packet.h"

#define __DEBUG_MODE__
#include "debug.h"

//#include <sstream>
//#include <iomanip>

namespace Barry {

//
// Controller constructor
//
/// Constructor for the Controller class.  Requires a valid ProbeResult
/// object to find the USB device to talk to.
///
/// \param[in]	device		One of the ProbeResult objects from the
///				Probe class.
///
Controller::Controller(const ProbeResult &device)
	: m_dev(device.m_dev)
	, m_iface(0)
	, m_pin(device.m_pin)
	, m_zero(m_dev, device.m_ep.write, device.m_ep.read, device.m_zeroSocketSequence)
//	m_mode(Unspecified),
//	m_tmpModeSocket(0),
//	m_halfOpen(false)
{
	unsigned char cfg;
	if( !m_dev.GetConfiguration(cfg) )
		throw Usb::Error(m_dev.GetLastError(),
			"Controller: GetConfiguration failed");

	if( cfg != BLACKBERRY_CONFIGURATION ) {
		if( !m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
			throw Usb::Error(m_dev.GetLastError(),
				"Controller: SetConfiguration failed");
	}

	m_iface = new Usb::Interface(m_dev, device.m_interface);

	m_dev.ClearHalt(device.m_ep.read);
	m_dev.ClearHalt(device.m_ep.write);
}

Controller::~Controller()
{
//	// trap exceptions in the destructor
//	try {
//		// a non-default socket has been opened, close it
//		m_socket.Close();
//	}
//	catch( std::runtime_error &re ) {
//		// do nothing... log it?
//		dout("Exception caught in ~Socket: " << re.what());
//	}

	// cleanup the interface
	delete m_iface;

	// this happens when for some reason the Desktop mode
	// is not fully opened, but the device has already recommended
	// a socket to open... in this case, reset the device
	// in the hopes that on next open, it will be in a
	// recognizable state.
	//
	// FIXME - this should not be necessary, and someday we
	// we should figure out how to handle the "already open"
	// response we get for the Desktop
	//
/*
	if( m_halfOpen ) {
		dout("Controller object destroyed in halfopen state, resetting device");
		m_dev.Reset();
	}
*/
}

///////////////////////////////////////////////////////////////////////////////
// protected members

//
// Tells device which mode is desired, and returns the suggested
// socket ID to use for that mode.
//
uint16_t Controller::SelectMode(ModeType mode)
{
	// select mode
	Protocol::Packet packet;
	packet.socket = 0;
	packet.size = htobs(SB_MODE_PACKET_COMMAND_SIZE);
	packet.command = SB_COMMAND_SELECT_MODE;
	packet.u.socket.socket = htobs(SB_MODE_REQUEST_SOCKET);
	packet.u.socket.sequence = 0; // updated by Socket::Send()
	memset(packet.u.socket.u.mode.name, 0, sizeof(packet.u.socket.u.mode.name));

	char *modeName = (char *) packet.u.socket.u.mode.name;
	switch( mode )
	{
	case Bypass:
		strcpy(modeName, "RIM Bypass");
		break;

	case Desktop:
		strcpy(modeName, "RIM Desktop");
		break;

	case JavaLoader:
		strcpy(modeName, "RIM_JavaLoader");
		break;

	case UsbSerData:
		strcpy(modeName, "RIM_UsbSerData");
		break;

	default:
		throw std::logic_error("Controller: Invalid mode in SelectMode");
		break;
	}

	// send mode command before we open, as a default socket is socket 0
	Data command(&packet, btohs(packet.size));
	Data response;

	try {
		m_zero.Send(command, response);

		// get the data socket number
		// indicates the socket number that
		// should be used below in the Open() call
		Protocol::CheckSize(response, SB_MODE_PACKET_RESPONSE_SIZE);
		MAKE_PACKET(modepack, response);
		if( modepack->command != SB_COMMAND_MODE_SELECTED ) {
			eeout(command, response);
			throw Error("Controller: mode not selected");
		}

		// return the socket that the device is expecting us to use
		return btohs(modepack->u.socket.socket);
	}
	catch( Usb::Error & ) {
		eout("Controller: error setting desktop mode");
		eeout(command, response);
		throw;
	}
}


///////////////////////////////////////////////////////////////////////////////
// public API



//////////////////////////////////////////////////////////////////////////////
// UsbSerData mode - modem specific

#if 0
// can be called from separate thread
void Controller::SerialRead(Data &data, int timeout)
{
	if( m_mode != UsbSerData )
		throw std::logic_error("Wrong mode in SerialRead");

	m_socket.Receive(data, timeout);
}

// based on Rick Scott's XmBlackBerry's serdata.c
void Controller::SerialWrite(const Data &data)
{
	if( m_mode != UsbSerData )
		throw std::logic_error("Wrong mode in SerialWrite");

	if( data.GetSize() <= 0 )
		return;	// nothing to do

	int size = data.GetSize() + 4;
	unsigned char *buf = m_writeCache.GetBuffer(size);
	MAKE_PACKETPTR_BUF(spack, buf);

	// copy data over to cache packet
	memcpy(&buf[4], data.GetData(), data.GetSize());

	// setup header
	spack->socket = htobs(m_socket.GetSocket());
	spack->size = htobs(size);

	// release and send
	m_writeCache.ReleaseBuffer(size);
	m_socket.Send(m_writeCache);

/*
	unsigned char buf[0x400];
	int num_read;
	int i;

	//
	// This is pretty ugly, but I have to put the HDLC flags into
	// the packets. RIM seems to need flags around every frame, and
	// a flag _cannot_ be an end and a start flag.
	//
	for (i = 0; i < num_read; i++) {
		BufferAdd(&serdata->data, &buf[i], 1);
		if (BufferData(&serdata->data)[0] == 0x7e && buf[i] == 0x7e) {
			if (BufferLen(&serdata->data) > 1 &&
				BufferData(&serdata->data)[0] == 0x7e && 
				BufferData(&serdata->data)[1] == 0x7e)
			{
				BufferPullHead(&serdata->data, 1);
			}
			else
			{
			}
			if (BufferLen(&serdata->data) > 2)
			{
				if ((BufferLen(&serdata->data) + 4) % 16 == 0)
				{
					BufferAdd(&serdata->data, (unsigned char *)"\0", 1);
				}
				send_packet(serdata, BufferData(&serdata->data), BufferLen(&serdata->data));
				BufferEmpty(&serdata->data);
				BufferAdd(&serdata->data, (unsigned char *)"\176", 1);
			}
			if (BufferLen(&serdata->data) == 2)
			{
				BufferPullTail(&serdata->data, 1);
			}
			else
			{
			}
		}
		else
		{
		}
	}
	if (BufferData(&serdata->data)[0] == 0x7e &&
	   memcmp(&BufferData(&serdata->data)[1], "AT", 2) == 0)
	{
		BufferPullHead(&serdata->data, 1);
	}
	if (BufferData(&serdata->data)[0] != 0x7e)
	{
		debug(9, "%s:%s(%d) - %i\n",
			__FILE__, __FUNCTION__, __LINE__,
			BufferLen(&serdata->data));
		send_packet(serdata, BufferData(&serdata->data), BufferLen(&serdata->data));
		BufferEmpty(&serdata->data);
	}
*/
}
#endif


} // namespace Barry

