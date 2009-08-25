///
/// \file	DeviceBus.cc
///		Bus for manipulating devices
///

/*
    Copyright (C) 2007-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "DeviceBus.h"

DeviceBus::DeviceBus()
{
}

DeviceBus::~DeviceBus()
{
}

//////////////////////////////////////////////////////////////////////////////
// Public API

void DeviceBus::Probe()
{
	m_probe.reset(new Barry::Probe);
}

unsigned int DeviceBus::ProbeCount()
{
	return m_probe->GetCount();
}

Device DeviceBus::Get(unsigned int i)
{
	return Device(m_probe->Get(i));
}
