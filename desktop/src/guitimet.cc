///
/// \file	guitimet.cc
///		Object to hold the date/time data of a set of GUI controls
///

/*
    Copyright (C) 2012-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <wx/wx.h>
#include "guitimet.h"

void GUITimeT::Set(time_t t)
{
	m_date.Set(t);
	m_hour = m_date.GetHour();
	m_min = m_date.GetMinute();
}

time_t GUITimeT::Get() const
{
	wxDateTime tmp = m_date.GetDateOnly();
	tmp.SetHour(m_hour);
	tmp.SetMinute(m_min);
	return tmp.GetTicks();
}

