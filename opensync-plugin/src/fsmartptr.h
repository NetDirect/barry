///
/// \file	fsmartptr.h
///		C++ smart pointer template that can call unique free
///		functions.
///

/*
    Copyright (C) 2007,
    	Net Direct Inc. (http://www.netdirect.ca/),
	and Chris Frey <cdfrey@foursquare.net>

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

#ifndef __REUSE_FSMARTPTR_H__
#define __REUSE_FSMARTPTR_H__

// A special smart pointer for pointer handling.
// Behaves like std::auto_ptr<> in that only one object
// at a time owns the pointer, and destruction frees it.
template <class T, class FT, void (*FreeFunc)(FT *pt)>
class fSmartPtr
{
	mutable T *m_pt;

public:
	fSmartPtr() : m_pt(0) {}
	fSmartPtr(T *pt) : m_pt(pt) {}
	fSmartPtr(const fSmartPtr &sp) : m_pt(sp.m_pt)
	{
		sp.m_pt = 0;
	}
	~fSmartPtr()
	{
		if( m_pt )
			FreeFunc(m_pt);
	}

	fSmartPtr& operator=(T *pt)
	{
		Extract();
		m_pt = pt;
		return *this;
	}

	fSmartPtr& operator=(const fSmartPtr &sp)
	{
		Extract();
		m_pt = sp.Extract();
		return *this;
	}

	T* Extract()
	{
		T *rp = m_pt;
		m_pt = 0;
		return rp;
	}

	T* Get()
	{
		return m_pt;
	}
};

#endif

