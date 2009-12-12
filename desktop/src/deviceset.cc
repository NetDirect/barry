///
/// \file	deviceset.cc
///		Class which detects a set of available or known devices
///		in an opensync-able system.
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "deviceset.h"

/// Does a USB probe automatically
DeviceSet::DeviceSet(OpenSync::APISet &apiset)
	: m_apiset(apiset)
{
}

/// Skips the USB probe and uses the results set given
DeviceSet(const Barry::Probe::Results &results, OpenSync::APISet &apiset)
	: m_apiset(apiset)
{
}

