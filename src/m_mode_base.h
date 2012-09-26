///
/// \file	m_mode_base.h
///		Base for mode classes
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

#ifndef __BARRY_M_MODE_BASE_H__
#define __BARRY_M_MODE_BASE_H__

#include "dll.h"
#include "controller.h"

namespace Barry {

namespace Mode {

//
// Mode class
//
/// Base class for simple mode classes.  Put common code here.
///
class BXEXPORT Mode
{
protected:
	Controller &m_con;
	Controller::ModeType m_modetype;
	SocketHandle m_socket;

	uint16_t m_ModeSocket;			// socket recommended by device
						// when mode was selected

public:
	Mode(Controller &con, Controller::ModeType type);
	virtual ~Mode();

	//////////////////////////////////
	// primary operations - required before anything else

	void Open(const char *password = 0);
	void Open(const char *password, const char *name);
	void RetryPassword(const char *password);
//	void Close();

protected:
	//////////////////////////////////
	// overrides

	virtual void OnOpen();
	virtual SocketRoutingQueue::SocketDataHandlerPtr GetHandler();
};

}} // namespace Barry::Mode

#endif

