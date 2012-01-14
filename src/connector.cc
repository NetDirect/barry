///
/// \file	connector.cc
///		Base class interface for handling Mode connections to device
///

/*
    Copyright (C) 2011-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "connector.h"
#include "router.h"
#include "controller.h"
#include "m_desktop.h"
#include "debug.h"

using namespace std;
using namespace Barry;

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// Connector base class

// we use a const char *password here because we don't want the
// responsibility of clear its memory... that's the application's job
Connector::Connector(const char *password,
		     const std::string &locale,
		     Barry::Pin pin)
	: m_password(password)
	, m_needs_reconnect(false)
	, m_ic(locale.c_str())
	, m_probe_result(FindDevice(pin))
	, m_connect_count(0)
	, m_last_disconnect(0)
	, m_bpcopy("", 0, 0)
{
}

Connector::Connector(const char *password,
		     const std::string &locale,
		     const Barry::ProbeResult &result)
	: m_password(password)
	, m_needs_reconnect(false)
	, m_ic(locale.c_str())
	, m_probe_result(result)
	, m_connect_count(0)
	, m_last_disconnect(0)
	, m_bpcopy("", 0, 0)
{
}

Connector::~Connector()
{
}

Barry::ProbeResult Connector::FindDevice(Barry::Pin pin)
{
	Barry::Probe probe;
	int i = probe.FindActive(pin);
	if( i != -1 )
		return probe.Get(i);
	else
		throw Barry::PinNotFound(pin, probe.GetCount());
}

void Connector::ClearPassword()
{
	// blank the memory first
	size_t len = m_password.size();
	while( len ) {
		len--;
		m_password[len] = '0';
	}

	// free it
	m_password.clear();
}

void Connector::SetPassword(const char *password)
{
	ClearPassword();
	m_password = password;
}

bool Connector::Connect()
{
	Disconnect();

	bool started = false;
	for(;;) {

		try {
			if( !started ) {
				started = true;
				StartConnect(m_password.c_str());
			}
			else {
				RetryPassword(m_password.c_str());
			}

			FinishConnect();
			m_connect_count++;
			return true;

		}
		catch( BadPassword &bp ) {
			if( bp.out_of_tries() ) {
				throw;
			}

			m_bpcopy = bp;

			// fall through to password prompt
		}

		// ask user for device password
		ClearPassword();
		if( !PasswordPrompt(m_bpcopy, m_password) ) {
			// user wants out
			return false;
		}
	}
}

void Connector::Disconnect()
{
	m_needs_reconnect = false;
	if( !IsDisconnected() ) {
		DoDisconnect();
		m_last_disconnect = time(NULL);
	}
}

bool Connector::Reconnect(int total_tries)
{
	int tries = 0;

	while(1) try {

		tries++;

		Disconnect();

		if( m_connect_count ) {
			// let the device settle... this seems to help prevent
			// the firmware hang, and therefore ultimately speeds
			// up the sync
			if( (time(NULL) - m_last_disconnect) < 2 ) {
				// don't bother sleeping if 2 seconds have
				// already passed
				sleep(1);
			}

			// temporary fix for odd reconnect message...
			// without this probe, the reconnect will often fail on
			// newer Blackberries due to an unexpected close socket
			// message.
			//
			// It is unclear if this is really a message from
			// the device, but until then, we add this probe.
			m_probe_result = FindDevice(m_probe_result.m_pin);
		}

		return Connect();
	}
	catch( Usb::Timeout & ) {
		if( tries >= total_tries ) {
			throw;
		}
		else {
			dout("Timeout in Connector::Reconnect()... trying again");
		}
	}
}

bool Connector::ReconnectForDirtyFlags()
{
	if( m_needs_reconnect ) {
		return Reconnect();
	}
	else {
		return true;
	}
}

void Connector::RequireDirtyReconnect()
{
	m_needs_reconnect = true;
}


//////////////////////////////////////////////////////////////////////////////
// DesktopConnector class

DesktopConnector::DesktopConnector(const char *password,
				const std::string &locale,
				Barry::Pin pin,
				Barry::SocketRoutingQueue *router,
				int connect_timeout)
	: Connector(password, locale, pin)
	, m_router(router)
	, m_connect_timeout(connect_timeout)
{
}

DesktopConnector::DesktopConnector(const char *password,
				const std::string &locale,
				const Barry::ProbeResult &result,
				Barry::SocketRoutingQueue *router,
				int connect_timeout)
	: Connector(password, locale, result)
	, m_router(router)
	, m_connect_timeout(connect_timeout)
{
}

void DesktopConnector::StartConnect(const char *password)
{
	// Note that there is a firmware issue that causes the firmware
	// to sometimes hang during a connect and it fails to respond
	// to a Desktop::Open() call.  To work around this, set the
	// timeout to something lower than the usual 30 seconds.
	// The default in DesktopConnector is 10 seconds, which should
	// be fine.
	//
	// If this bug triggers, a Timeout exception will be thrown,
	// which will be caught by the Reconnect() method, and Reconnect()
	// will retry according to the total number of retries it is
	// set to do.
	//
	if( m_router ) {
		m_con.reset( new Barry::Controller(m_probe_result,
						*m_router, m_connect_timeout) );
	}
	else {
		m_con.reset( new Barry::Controller(m_probe_result,
						m_connect_timeout) );
	}
	m_desktop.reset( new Barry::Mode::Desktop(*m_con, m_ic) );
	m_desktop->Open(password);
}

void DesktopConnector::RetryPassword(const char *password)
{
	m_desktop->RetryPassword(password);
}

void DesktopConnector::FinishConnect()
{
}

void DesktopConnector::DoDisconnect()
{
	m_desktop.reset();
	m_con.reset();
}

bool DesktopConnector::IsDisconnected()
{
	// return true if DoDisconnect can safely be skipped
	return !m_con.get() && !m_desktop.get();
}

bool DesktopConnector::IsConnected()
{
	if( m_con.get() && m_desktop.get() )
		return true;
	return false;
}

}

