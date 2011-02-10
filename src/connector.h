//
// \file	connector.h
//		Base class interface for handling Mode connections to device
//

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_CONNECT_H__
#define __BARRY_CONNECT_H__

#include "dll.h"
#include "iconv.h"
#include "pin.h"
#include "probe.h"
#include <string>
#include <memory>
#include <time.h>

namespace Barry {

class SocketRoutingQueue;
class Controller;
namespace Mode {
	class Desktop;
}

class BXEXPORT Connector
{
protected:
	std::string m_password;
	bool m_needs_reconnect;
	Barry::IConverter m_ic;
	Barry::ProbeResult m_probe_result;
	int m_connect_count;
	time_t m_last_disconnect;

protected:
	// helper functions
	static Barry::ProbeResult FindDevice(Barry::Pin pin);

	// required overrides by derived classes
	virtual void StartConnect(const char *password) = 0;
	virtual void RetryPassword(const char *password) = 0;
	virtual void FinishConnect() = 0;
	virtual void DoDisconnect() = 0;
	/// slightly different than IsConnected()... this returns true
	/// even if there is a partial connection in progress...
	/// i.e. this returns true if DoDisconnect() can be safely skipped
	virtual bool IsDisconnected() = 0;

public:
	Connector(const char *password, const std::string &locale,
		Barry::Pin pin = 0);
	Connector(const char *password, const std::string &locale,
		const Barry::ProbeResult &result);
	virtual ~Connector();

	IConverter& GetIConverter() { return m_ic; }
	const IConverter& GetIConverter() const { return m_ic; }
	Barry::ProbeResult& GetProbeResult() { return m_probe_result; }
	const Barry::ProbeResult& GetProbeResult() const { return m_probe_result; }

	virtual void ClearPassword();
	virtual void SetPassword(const char *password);

	/// Returns true if connected, false if user cancelled, throws
	/// Barry exception on error.
	virtual bool Connect();

	/// Disconnects from the device
	virtual void Disconnect();

	/// Returns same as Connect(), but normally remembers the password
	/// and so avoids prompting the user if possible.  Password prompts
	/// are still possible though, if you have called ClearPassword().
	///
	/// It is valid to call Reconnect() without ever calling Connect(),
	/// since Reconnect() is simply a wrapper that handles retries.
	virtual bool Reconnect(int total_tries = 2);

	/// Calls Reconnect() (and returns it's result) only if you have
	/// called RequireDirtyReconnect().  Otherwise, does nothing, but
	/// returns true.
	virtual bool ReconnectForDirtyFlags();

	/// Returns true if connected, false if not
	virtual bool IsConnected() = 0;

	/// This function flags the Connector object so that a future
	/// call to ReconnectForDirtyFlags() will actually Reconnect().
	/// This is needed in cases where you are updating the device,
	/// and require that the dirty flags on the device itself are
	/// properly cleared and updated.  In this case, you must call
	/// ReconnectForDirtyFlags() before Desktop::GetRecordStateTable().
	/// Disconnecting from the device, or reconnecting, clears the flag.
	virtual void RequireDirtyReconnect();

	//
	// Callbacks, overridden by the application
	//

	/// App should prompt user for password, fill password_result with
	/// what he enters and then return true.  Return false if user
	/// wishes to stop trying.
	///
	/// This function is *not* called from inside a catch() routine,
	/// so it is safe to throw exceptions from it if you must.
	virtual bool PasswordPrompt(const Barry::BadPassword &bp,
					std::string &password_result) = 0;
};

class BXEXPORT DesktopConnector : public Connector
{
	Barry::SocketRoutingQueue *m_router;
	std::auto_ptr<Barry::Controller> m_con;
	std::auto_ptr<Mode::Desktop> m_desktop;
	int m_connect_timeout;

protected:
	virtual void StartConnect(const char *password);
	virtual void RetryPassword(const char *password);
	virtual void FinishConnect();
	virtual void DoDisconnect();
	virtual bool IsDisconnected();

public:
	// Override the timeout due to a firmware issue... sometimes
	// the firmware will hang during a Reconnect, and fail to
	// respond to a Desktop::Open().  To work around this, we
	// set the default timeout to 10 seconds so that we find this
	// failure early enough to fix it within opensync's 30 second timeout.
	// Then if we get such a timeout, we do the Reconnect again and
	// hope for the best... this often fixes it.
	//
	DesktopConnector(const char *password, const std::string &locale,
		Barry::Pin pin = 0, Barry::SocketRoutingQueue *router = 0,
		int connect_timeout = 10000);

	DesktopConnector(const char *password, const std::string &locale,
		const Barry::ProbeResult &result,
		Barry::SocketRoutingQueue *router = 0,
		int connect_timeout = 10000);

	virtual bool IsConnected();

	virtual bool PasswordPrompt(const Barry::BadPassword &bp,
					std::string &password_result)
	{
		// default to only trying the existing password once
		return false;
	}

	//
	// Do not use these functions if IsConnected() returns false
	//

	Controller& GetController() { return *m_con; }
	Mode::Desktop& GetDesktop() { return *m_desktop; }

	const Controller& GetController() const { return *m_con; }
	const Mode::Desktop& GetDesktop()  const{ return *m_desktop; }
};

}

#endif

