///
/// \file	controllerpriv.h
///		Private data for the Controller class
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

#ifndef __BARRY_CONTROLLERPRIVATE_H__
#define __BARRY_CONTROLLERPRIVATE_H__

#include "probe.h"
#include "pin.h"
#include "socket.h"
#include "router.h"
#include "m_ipmodem.h"

namespace Barry {

class Controller;

class PrivateControllerData
{
	friend class Controller;

	// DO NOT add your Mode class to this list unless it *needs*
	// low level access to things like Usb::Device.  By adding
	// your Mode class to this list, you are making it non-portable.
	friend class Barry::Mode::IpModem;


private:
	ProbeResult m_result;
	Usb::Device m_dev;
	Usb::Interface *m_iface;
	Pin m_pin;
	SocketZero m_zero;
	SocketRoutingQueue *m_queue;	//< ptr to external object; no delete

private:
	PrivateControllerData(const ProbeResult &device, int default_timeout)
		: m_result(device)
		, m_dev(device.m_dev, default_timeout)
		, m_iface(0)
		, m_pin(device.m_pin)
		, m_zero(m_dev, device.m_ep.write, device.m_ep.read, device.m_zeroSocketSequence)
		, m_queue(0)
	{
	}

	PrivateControllerData(const ProbeResult &device,
		SocketRoutingQueue &queue, int default_timeout)
		: m_result(device)
		, m_dev(device.m_dev, default_timeout)
		, m_iface(0)
		, m_pin(device.m_pin)
		, m_zero(queue, device.m_ep.write, device.m_zeroSocketSequence)
		, m_queue(&queue)
	{
	}

public:
	~PrivateControllerData()
	{
		// detach the router from our device
		if( m_queue ) {
			m_queue->ClearUsbDevice();
			m_queue = 0;
		}

		// cleanup the interface
		delete m_iface;
	}
};

} // namespace Barry

#endif
