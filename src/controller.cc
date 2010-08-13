///
/// \file	controller.cc
///		High level Barry API class
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "probe.h"
#include "common.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "endian.h"
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
	: m_result(device)
	, m_dev(device.m_dev, default_timeout)
	, m_iface(0)
	, m_pin(device.m_pin)
	, m_zero(m_dev, device.m_ep.write, device.m_ep.read, device.m_zeroSocketSequence)
	, m_queue(0)
{
	dout("Controller: Using non-threaded sockets");
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
	: m_result(device)
	, m_dev(device.m_dev, default_timeout)
	, m_iface(0)
	, m_pin(device.m_pin)
	, m_zero(queue, device.m_ep.write, device.m_zeroSocketSequence)
	, m_queue(&queue)
{
	dout("Controller: Using threaded socket router");

	SetupUsb(device);

	// set the queue to use our device
	queue.SetUsbDevice(&m_dev, device.m_ep.write, device.m_ep.read);
}

void Controller::SetupUsb(const ProbeResult &device)
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

	// detach the router from our device
	if( m_queue ) {
		m_queue->ClearUsbDevice();
		m_queue = 0;
	}

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
	// FIXME - halfOpen now seems to be handled in the Socket class...
	// perhaps move this there if needed
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

	case JVMDebug:
		strcpy(modeName, "RIM_JVMDebug");
		break;

	case UsbSerData:
		strcpy(modeName, "RIM_UsbSerData");
		break;

	case UsbSerCtrl:
		strcpy(modeName, "RIM_UsbSerCtrl");
		break;

	case VNCServer:
		strcpy(modeName, "VNC_Server");
		break;

	default:
		throw std::logic_error("Controller: Invalid mode in SelectMode");
		break;
	}

    std::cerr << "a\n";
	// send mode command before we open, as a default socket is socket 0
	Data command(&packet, btohs(packet.size));
	Data response;

    std::cerr << "b\n";

	try {
		m_zero.Send(command, response);

		// get the data socket number
		// indicates the socket number that
		// should be used below in the Open() call
    std::cerr << "c\n";
		//Protocol::CheckSize(response, SB_MODE_PACKET_RESPONSE_SIZE);
    std::cerr << "d\n";
		MAKE_PACKET(modepack, response);
		if( modepack->command != SB_COMMAND_MODE_SELECTED ) {
			eeout(command, response);
			throw Error("Controller: mode not selected");
		}
    std::cerr << "e\n";

		if (mode == Desktop) {
			// On the BlackBerry Storm, I have to read a packet
			// after opening a socket. (only for Desktop mode)
			// Otherwise, barrybackup and opensync-plugin can crash (timeout)
			// I don't know why ! Maybe a bug on the handheld.
			m_zero.HideSequencePacket(false);
			m_zero.Receive(response);
			m_zero.HideSequencePacket(true);
		}
		std::cerr << "f\n";
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


} // namespace Barry

