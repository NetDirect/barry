///
/// \file	controller.h
///		High level BlackBerry API class
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

#ifndef __BARRY_CONTROLLER_H__
#define __BARRY_CONTROLLER_H__

#include "usbwrap.h"
#include "socket.h"
//#include "probe.h"
//#include "record.h"
//#include "data.h"

/// Project namespace, containing all related functions and classes.
/// This is the only namespace applications should be concerned with,
/// for now.
namespace Barry {

// forward declarations
class Parser;
class Builder;
class DBPacket;

namespace Mode {
	class Desktop;
	class Serial;
}

//
// Controller class
//
/// The main interface class.  This class coordinates the communication to
/// a single handheld.
///
/// To use this class, use the following steps:
///
///	- Probe the USB bus for matching devices with the Probe class
///	- Pass one of the probe results into the Controller constructor
///		to connect
fixme... openmode is no longer appropriate
///	- Call OpenMode() to select the desired mode.  This will fill all
///		internal data structures for that mode, such as the
///		Database Database in Desktop mode.
///		NOTE: only Desktop mode is currently implemented.
///	- Call GetDBDB() to get the device's database database
///	- Call GetDBID() to get a database ID by name
///	- In Desktop mode, call LoadDatabase() to retrieve and store a database
///
class Controller
{
//	friend class Barry::Mode::Desktop;
//	friend class Barry::Mode::Serial;

public:
	/// Handheld mode type
	enum ModeType {
		Unspecified,		//< default on start up
		Bypass,			//< unsupported, unknown
		Desktop,		//< desktop mode required for database
					//< operation
		JavaLoader,		//< unsupported
		UsbSerData,		//< GPRS modem support over USB
		UsbSerCtrl		//< internally used behind the scenes
	};

private:
	Usb::Device m_dev;
	Usb::Interface *m_iface;
	uint32_t m_pin;

	SocketZero m_zero;
//	SocketHandle m_serCtrlSocket;

//	CommandTable m_commandTable;
//	DatabaseDatabase m_dbdb;

//	ModeType m_mode;

//	uint16_t m_tmpModeSocket;		// socket recommended by device
						// when mode was selected

	// UsbSerData cache
//	Data m_writeCache, m_readCache;

	// tracking of open Desktop socket, and the need to reset
//	bool m_halfOpen;

protected:
	uint16_t SelectMode(ModeType mode);	// returns mode socket
	SocketRoutingQueue* GetRoutingQueue();

public:
	explicit Controller(const ProbeResult &device);
	Controller(const ProbeResult &device, SocketRoutingQueue &queue);
	~Controller();
};

} // namespace Barry

#endif

