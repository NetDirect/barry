///
/// \file	controller.cc
///		High level Barry API class
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "i18n.h"
#include "controller.h"
#include "controllerpriv.h"
#include "common.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "endian.h"
#include "platform.h"
#include <string.h>

#define __DEBUG_MODE__
#include "debug.h"

namespace Barry {

//
// Controller constructor
//
/// Constructor for the Controller class.  Requires a valid ProbeResult
/// object to find the USB device to talk to.
///
/// \param[in]	device		One of the ProbeResult objects from the
///				Probe class.
/// \param[in]	default_timeout	Override Usb::Device's default timeout
///
Controller::Controller(const ProbeResult &device,
			int default_timeout)
	: m_priv(new PrivateControllerData(device, default_timeout))
{
	dout(_("Controller: Using non-threaded sockets"));
	SetupUsb(device);
}

//
// Controller constructor
//
/// Constructor for the Controller class.  Requires a valid ProbeResult
/// object to find the USB device to talk to.
///
/// \param[in]	device		One of the ProbeResult objects from the
///				Probe class.
/// \param[in]	queue		Plugin router object for reading data
///				from sockets.
/// \param[in]	default_timeout	Override Usb::Device's default timeout
///
Controller::Controller(const ProbeResult &device,
			SocketRoutingQueue &queue,
			int default_timeout)
	: m_priv(new PrivateControllerData(device, queue, default_timeout))
{
	dout(_("Controller: Using threaded socket router"));

	SetupUsb(device);

	// set the queue to use our device
	queue.SetUsbDevice(&m_priv->m_dev, device.m_ep.write, device.m_ep.read);
}

void Controller::SetupUsb(const ProbeResult &device)
{
	unsigned char cfg;
	if( !m_priv->m_dev.GetConfiguration(cfg) )
		throw Usb::Error(m_priv->m_dev.GetLastError(),
			_("Controller: GetConfiguration failed"));

	if( cfg != BLACKBERRY_CONFIGURATION || MUST_SET_CONFIGURATION ) {
		if( !m_priv->m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
			throw Usb::Error(m_priv->m_dev.GetLastError(),
				_("Controller: SetConfiguration failed"));
	}

	m_priv->m_iface = new Usb::Interface(m_priv->m_dev, device.m_interface);

	if( device.m_needSetAltInterface ) {
		m_priv->m_iface->SetAltInterface(device.m_altsetting);
	}

	if( device.m_needClearHalt ) {
		m_priv->m_dev.ClearHalt(device.m_ep.read);
		m_priv->m_dev.ClearHalt(device.m_ep.write);
	}
}

Controller::~Controller()
{
}

///////////////////////////////////////////////////////////////////////////////
// protected members

//
// Tells device which mode is desired, and returns the suggested
// socket ID to use for that mode.
//
uint16_t Controller::SelectMode(ModeType mode)
{
	return SelectMode(mode, NULL);
}
//
// Tells device which mode is desired, and returns the suggested
// socket ID to use for that mode.
//
// If explicitModeName is not NULL then it will be used as the mode name.
// Otherwise the default mode name for the given mode will be used.
// It should be a nul terminated string if it is provided.
//
// The RawChannel mode requires an explicitModeName to be specified.
//
uint16_t Controller::SelectMode(ModeType mode, const char *explicitModeName)
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

	if( explicitModeName ) {
		if( strlen(explicitModeName) >= sizeof(packet.u.socket.u.mode.name) ) {
			throw std::logic_error(_("Controller: explicit mode name too long"));
		}
		strcpy(modeName, explicitModeName);
	}
	else {
		// No modeName given, use the default
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

		case JVMDebug:
			strcpy(modeName, "RIM_JVMDebug");
			break;

		case UsbSerData:
			strcpy(modeName, "RIM_UsbSerData");
			break;

		case UsbSerCtrl:
			strcpy(modeName, "RIM_UsbSerCtrl");
			break;

		case RawChannel:
			throw std::logic_error(_("Controller: No channel name given with RawChannel mode"));
			break;

		default:
			throw std::logic_error(_("Controller: Invalid mode in SelectMode"));
			break;
		}
	}

	// send mode command before we open, as a default socket is socket 0
	Data command(&packet, btohs(packet.size));
	Data response;

	try {
		m_priv->m_zero.Send(command, response);

		// get the data socket number
		// indicates the socket number that
		// should be used below in the Open() call
		MAKE_PACKET(modepack, response);
		if( modepack->command == SB_COMMAND_MODE_NOT_SELECTED ) {
		        throw Error(_("Controller: requested mode not supported"));
		}
		if( modepack->command != SB_COMMAND_MODE_SELECTED ) {
			eeout(command, response);
			throw Error(_("Controller: mode not selected"));
		}

		// return the socket that the device is expecting us to use
		return btohs(modepack->u.socket.socket);
	}
	catch( Usb::Error & ) {
		eout(_("Controller: error setting desktop mode"));
		eeout(command, response);
		throw;
	}
}

//
// OpenSocket
//
/// Can be called multiple times, in case of password retries.
/// See also Mode::RetryPassword()
///
SocketHandle Controller::OpenSocket(uint16_t socket, const char *password)
{
	return m_priv->m_zero.Open(socket, password);
}

//
// OpenSocket
//
/// Sets a data handler from the start of the socket being opened.
/// This avoids a race condition where data packets can be lost
/// from the device if they are received before the mode which owns
/// this socket calls Socket::RegisterInterest().
///
/// Can be called multiple times, in case of password retries.
/// See also Mode::RetryPassword()
///
SocketHandle Controller::OpenSocket(
	SocketRoutingQueue::SocketDataHandlerPtr handler,
	uint16_t socket, const char *password)
{
	return m_priv->m_zero.Open(handler, socket, password);
}


///////////////////////////////////////////////////////////////////////////////
// public API

bool Controller::HasQueue() const
{
	return m_priv->m_queue != NULL;
}

SocketRoutingQueue* Controller::GetQueue()
{
	return m_priv->m_queue;
}

const ProbeResult& Controller::GetProbeResult() const
{
	return m_priv->m_result;
}

} // namespace Barry

