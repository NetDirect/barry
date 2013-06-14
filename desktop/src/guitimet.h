///
/// \file	guitimet.h
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

#ifndef __BARRYDESKTOP_GUITIMET_H__
#define __BARRYDESKTOP_GUITIMET_H__

class GUITimeT
{
public:
	wxDateTime m_date;
	int m_hour;
	int m_min;

public:
	GUITimeT()
		: m_hour(0)
		, m_min(0)
	{
	}

	explicit GUITimeT(time_t t)
		: m_hour(0)
		, m_min(0)
	{
		Set(t);
	}

	void Set(time_t t);
	time_t Get() const;
};

#endif

