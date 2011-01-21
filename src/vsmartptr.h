///
/// \file	vsmartptr
///		Variable 'free' smart pointer
///

/*
    Copyright (C) 2006-2011, Net Direct Inc. (http://www.netdirect.ca/)

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
		reset();
	}

	vSmartPtr& operator=(T *pt)
	{
		reset(pt);
		return *this;
	}

	vSmartPtr& operator=(const vSmartPtr &sp)
	{
		reset(sp.release());
		return *this;
	}

	// Some non-standard APIs used by Barry
	T* Extract()
	{
		return this->release();
	}

	T* Get()
	{
		return this->get();
	}

	// std::auto_ptr<> style API
	T* get()
	{
		return m_pt;
	}

	T* release()
	{
		T *rp = m_pt;
		m_pt = 0;
		return rp;
	}

	void reset(T *new_obj = 0)
	{
		if( m_pt )
			FreeFunc(m_pt);
		m_pt = new_obj;
	}
};

//
// vLateSmartPtr
//
/// Variation of the above smart pointer that allows the user to
/// assign a free function after construction, in the case of
/// dlopen()'d frees.
///
template <class T, class FreeFuncPtrT>
class vLateSmartPtr
{
	mutable T *m_pt;
	FreeFuncPtrT m_FreeFuncPtr;

public:
	explicit vLateSmartPtr(FreeFuncPtrT freefunc = 0)
		: m_pt(0)
		, m_FreeFuncPtr(freefunc)
	{
	}

	vLateSmartPtr(T *pt, FreeFuncPtrT freefunc = 0)
		: m_pt(pt)
		, m_FreeFuncPtr(freefunc)
	{
	}

	vLateSmartPtr(const vLateSmartPtr &sp)
		: m_pt(sp.m_pt)
		, m_FreeFuncPtr(sp.m_FreeFuncPtr)
	{
		sp.m_pt = 0;
	}

	~vLateSmartPtr()
	{
		reset();
	}

	void SetFreeFunc(FreeFuncPtrT freefunc)
	{
		m_FreeFuncPtr = freefunc;
	}

	vLateSmartPtr& operator=(T *pt)
	{
		reset(pt);
		return *this;
	}

	vLateSmartPtr& operator=(const vLateSmartPtr &sp)
	{
		reset(sp.release());
		m_FreeFuncPtr = sp.m_FreeFuncPtr;
		return *this;
	}

	// Some non-standard APIs used by Barry
	T* Extract()
	{
		return this->release();
	}

	T* Get()
	{
		return this->get();
	}

	// std::auto_ptr<> style API
	T* get()
	{
		return m_pt;
	}

	T* release()
	{
		T *rp = m_pt;
		m_pt = 0;
		return rp;
	}

	void reset(T *new_obj = 0)
	{
		// don't check for null m_FreeFuncPtr, since
		// that should be an obvious crash and requires fixing
		if( m_pt )
			(*m_FreeFuncPtr)(m_pt);
		m_pt = new_obj;
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

