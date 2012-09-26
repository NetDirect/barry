///
/// \file	controller.h
///		High level BlackBerry API class
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

#ifndef __BARRY_CONTROLLER_H__
#define __BARRY_CONTROLLER_H__

#include "dll.h"
#include "socket.h"

/// Project namespace, containing all related functions and classes.
/// This is the only namespace applications should be concerned with,
/// for now.
namespace Barry {

// forward declarations
class SocketRoutingQueue;
struct ProbeResult;

class PrivateControllerData;

namespace Mode {
	class Mode;
	class IpModem;
	class Serial;
	class JavaLoader;
	class JVMDebug;
	class RawChannel;
}

//
// Controller class
//
/// The main interface class.  This class coordinates the communication to
/// a single handheld.  This class also owns the only Usb::Device object
/// the handheld.  All other classes reference this one for the low level
/// device object.  This class owns the only SocketZero object as well,
/// which is the object that any SocketRoutingQueue is plugged into
/// if constructed that way.
///
/// To use this class, use the following steps:
///
///	- Probe the USB bus for matching devices with the Probe class
///	- Create an optional SocketRoutingQueue object and create a
///		read thread for it, or use its default read thread.
///	- Pass one of the probe results into the Controller constructor
///		to connect to the USB device.  Pass the routing queue
///		to the Controller constructor here too, if needed.
///	- Create the Mode object of your choice.  See m_desktop.h
///		and m_serial.h for these mode classes.  You pass
///		your controller object into these mode constructors
///		to create the mode.
///
class BXEXPORT Controller
{
	friend class Barry::Mode::Mode;
	friend class Barry::Mode::IpModem;
	friend class Barry::Mode::Serial;
	friend class Barry::Mode::JavaLoader;
	friend class Barry::Mode::JVMDebug;
	friend class Barry::Mode::RawChannel;

public:
	/// Handheld mode type
	enum ModeType {
		Unspecified,		//< default on start up (unused)
		Bypass,			//< unsupported, unknown
		Desktop,		//< desktop mode required for database
					//< operation
		JavaLoader,		//< experimental
		JVMDebug,		//< experimental
		UsbSerData,		//< GPRS modem support over USB
		UsbSerCtrl,		//< internally used behind the scenes
		RawChannel		//< raw channel
	};

private:
	const std::auto_ptr<PrivateControllerData> m_priv;

private:
	Controller(const Controller& rhs); // prevent copying
	void SetupUsb(const ProbeResult &device);

protected:
	uint16_t SelectMode(ModeType mode);	// returns mode socket
	uint16_t SelectMode(ModeType mode, const char *explicitModeName); // returns mode socket
	SocketHandle OpenSocket(uint16_t socket, const char *password = 0);
	SocketHandle OpenSocket(
		SocketRoutingQueue::SocketDataHandlerPtr handler,
		uint16_t socket, const char *password = 0);
	PrivateControllerData* GetPrivate() { return m_priv.get(); }

public:
	explicit Controller(const ProbeResult &device,
		int default_timeout = USBWRAP_DEFAULT_TIMEOUT);
	Controller(const ProbeResult &device, SocketRoutingQueue &queue,
		int default_timeout = USBWRAP_DEFAULT_TIMEOUT);
	~Controller();

	bool HasQueue() const;
	SocketRoutingQueue* GetQueue();	// FIXME - not really ideal to have
					// this exposed, but oh well

	const ProbeResult& GetProbeResult() const;
};

} // namespace Barry

#endif

