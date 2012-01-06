///
/// \file	EasyCondition.h
///		Wrapper thread around wxMutex and wxCondition, to make
///		simple waits easy and safe.  Also an RAII scope signaler.
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_EASYCONDITION_H__
#define __BARRYDESKTOP_EASYCONDITION_H__

#include <wx/wx.h>

class EasyCondition
{
	wxMutex m_mutex;
	wxCondition m_condition;

public:
	EasyCondition()
		: m_condition(m_mutex)
	{
		m_mutex.Lock();
	}

	void Wait()
	{
		m_condition.Wait();
	}

	void Signal()
	{
		wxMutexLocker lock(m_mutex);
		m_condition.Broadcast();
	}
};

class ScopeSignaler
{
	EasyCondition &m_condition;

public:
	explicit ScopeSignaler(EasyCondition &cond)
		: m_condition(cond)
	{
	}

	~ScopeSignaler()
	{
		m_condition.Signal();
	}
};

#endif

