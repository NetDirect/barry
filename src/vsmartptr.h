///
/// \file	vsmartptr
///		Variable 'free' smart pointer
///

/*
    Copyright (C) 2006-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_VSMARTPTR_H__
#define __BARRY_VSMARTPTR_H__

namespace Barry {

//
// vSmartPtr
//
/// A special smart pointer for variables that have their own
/// special 'free' functions.  Behaves like std::auto_ptr<>
/// in that only one object at a time owns the pointer,
/// and destruction frees it by calling the given FreeFunc.
///
template <class T, class FT, void (*FreeFunc)(FT *pt)>
class vSmartPtr
{
	mutable T *m_pt;

public:
	vSmartPtr() : m_pt(0) {}
	vSmartPtr(T *pt) : m_pt(pt) {}
	vSmartPtr(const vSmartPtr &sp) : m_pt(sp.m_pt)
	{
		sp.m_pt = 0;
	}
	~vSmartPtr()
	{
		if( m_pt )
			FreeFunc(m_pt);
	}

	vSmartPtr& operator=(T *pt)
	{
		Extract();
		m_pt = pt;
		return *this;
	}

	vSmartPtr& operator=(const vSmartPtr &sp)
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

/*

Example usage:

typedef vSmartPtr<b_VFormatAttribute, b_VFormatAttribute, &b_vformat_attribute_free> vAttrPtr;
typedef vSmartPtr<b_VFormatParam, b_VFormatParam, &b_vformat_attribute_param_free> vParamPtr;
typedef vSmartPtr<char, void, &g_free> gStringPtr;

*/

} // namespace Barry

#endif

