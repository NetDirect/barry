///
/// \file	vbase.h
///		Base class for vformat support
///

/*
    Copyright (C) 2006-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_VBASE_H__
#define __BARRY_SYNC_VBASE_H__

#include <string>
#include <stdexcept>
#include "vformat.h"

// forward declarations
class BarryEnvironment;


// FIXME - possibly scrap this in favour of opensync's xml time routines,
// but this will require an overhaul of the plugin itself.
class vTimeZone
{
public:
	struct ZoneBlock
	{
		bool m_daylightSaving;
		std::string m_name;
		std::string m_dtstart;
		std::string m_tzoffsetfrom;
		std::string m_tzoffsetto;
	};

private:
	bool m_valid;

public:
	std::string m_id;

public:
	vTimeZone();
	~vTimeZone();

	void Clear();

	bool IsValid();

	/// used for comparing by TZID
	bool operator==(const std::string &tzid) const;
};


// A special smart pointer for vformat pointer handling.
// Behaves like std::auto_ptr<> in that only one object
// at a time owns the pointer, and destruction frees it.
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

typedef vSmartPtr<VFormatAttribute, VFormatAttribute, &vformat_attribute_free> vAttrPtr;
typedef vSmartPtr<VFormatParam, VFormatParam, &vformat_attribute_param_free> vParamPtr;
typedef vSmartPtr<char, void, &g_free> gStringPtr;


//
// vAttr
//
/// Class for reading a VFormatAttribute.  Reading does not require
/// memory management, so none is done.
///
class vAttr
{
	VFormatAttribute *m_attr;

public:
	vAttr()
		: m_attr(0)
	{
	}

	vAttr(VFormatAttribute *attr)
		: m_attr(attr)
	{
	}

	vAttr& operator=(VFormatAttribute *attr)
	{
		m_attr = attr;
		return *this;
	}

	VFormatAttribute* Get() { return m_attr; }

	// These functions do not throw an error if the value
	// is NULL or does not exist (for example, if you ask for
	// value #5 and there are only 4).
	std::string GetName();
	std::string GetValue(int nth = 0);
	std::string GetParam(const char *name, int nth = 0);
};


//
// vBase
//
/// Base class containing vformat helper API.
///
class vBase
{
	// internal data for managing the vformat
	VFormat *m_format;

public:
	// FIXME - if you put this class in the Barry library,
	// you'll need to change the class hierarchy
	class ConvertError : public std::runtime_error
	{
	public:
		ConvertError(const std::string &msg) : std::runtime_error(msg) {}
	};

protected:
	vBase();
	virtual ~vBase();

	VFormat* Format() { return m_format; }
	const VFormat* Format() const { return m_format; }
	void SetFormat(VFormat *format);

	void Clear();

	vAttrPtr NewAttr(const char *name);
	vAttrPtr NewAttr(const char *name, const char *value);
	void AddAttr(vAttrPtr attr);
	void AddValue(vAttrPtr &attr, const char *value);
	void AddParam(vAttrPtr &attr, const char *name, const char *value);

	std::string GetAttr(const char *attrname, const char *block = 0);
	vAttr GetAttrObj(const char *attrname, int nth = 0, const char *block = 0);
};

#endif

