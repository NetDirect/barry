///
/// \file	DeviceBus.h
///		Bus for manipulating devices
///

/*
    Copyright (C) 2007-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_DEVICEBUS_H__
#define __BARRYBACKUP_DEVICEBUS_H__

#include <barry/barry.h>
#include "DeviceIface.h"

class DeviceBus
{
private:
	std::auto_ptr<Barry::Probe> m_probe;

public:
	DeviceBus();
	~DeviceBus();

	void Probe();
	unsigned int ProbeCount();
	Device Get(unsigned int index);
};

#endif
