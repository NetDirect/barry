//
// \file	barry_sync.h
//		Opensync module for the USB Blackberry handheld
//

/*
    Copyright (C) 2006, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_H__
#define __BARRY_SYNC_H__

#include <opensync/opensync.h>
#include <barry/barry.h>

struct BarryEnvironment
{
public:
	OSyncMember *member;

	//If you need a hashtable:
	OSyncHashTable *hashtable;

	Barry::ProbeResult m_ProbeResult;
	Barry::Controller *m_pCon;

	uint32_t m_pin;

public:
	BarryEnvironment()
		: member(0), hashtable(0), m_pCon(0), m_pin(0)
	{
	}

	~BarryEnvironment()
	{
		delete m_pCon;
	}

	void Disconnect()
	{
		delete m_pCon;
		m_pCon = 0;
	}
};

#endif

