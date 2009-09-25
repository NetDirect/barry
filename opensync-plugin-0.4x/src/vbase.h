///
/// \file	vbase.h
///		Base class for vformat support
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

#ifndef __BARRY_SYNC_VBASE_H__
#define __BARRY_SYNC_VBASE_H__

#include <string>
#include <vector>
#include <stdexcept>
#include <barry/vsmartptr.h>
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


typedef vSmartPtr<b_VFormatAttribute, b_VFormatAttribute, &b_vformat_attribute_free> vAttrPtr;
typedef vSmartPtr<b_VFormatParam, b_VFormatParam, &b_vformat_attribute_param_free> vParamPtr;
typedef vSmartPtr<char, void, &g_free> gStringPtr;


//
// vAttr
//
/// Class for reading a b_VFormatAttribute.  Reading does not require
/// memory management, so none is done.
///
class vAttr
{
	b_VFormatAttribute *m_attr;

public:
	vAttr()
		: m_attr(0)
	{
	}

	vAttr(b_VFormatAttribute *attr)
		: m_attr(attr)
	{
	}

	vAttr& operator=(b_VFormatAttribute *attr)
	{
		m_attr = attr;
		return *this;
	}

	b_VFormatAttribute* Get() { return m_attr; }

	// These functions do not throw an error if the value
	// is NULL or does not exist (for example, if you ask for
	// value #5 and there are only 4).
	std::string GetName();
	std::string GetValue(int nth = 0);
	std::string GetDecodedValue();
	std::string GetParam(const char *name, int nth = 0);
	std::string GetAllParams(const char *name);
};


//
// vBase
//
/// Base class containing vformat helper API.
///
class vBase
{
	// internal data for managing the vformat
	b_VFormat *m_format;

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

	b_VFormat* Format() { return m_format; }
	const b_VFormat* Format() const { return m_format; }
	void SetFormat(b_VFormat *format);

	void Clear();

	vAttrPtr NewAttr(const char *name);
	vAttrPtr NewAttr(const char *name, const char *value);
	void AddAttr(vAttrPtr attr);
	void AddValue(vAttrPtr &attr, const char *value);
	void AddEncodedValue(vAttrPtr &attr, b_VFormatEncoding encoding, const char *value, int len);
	void AddParam(vAttrPtr &attr, const char *name, const char *value);

	std::string GetAttr(const char *attrname, const char *block = 0);
	std::vector<std::string> GetValueVector(const char *attrname, const char *block = 0);
	vAttr GetAttrObj(const char *attrname, int nth = 0, const char *block = 0);

	std::vector<std::string> Tokenize(const std::string &str, const char delim = ',');
	std::string ToStringList(const std::vector<std::string> &list, const char delim = ',');

};

#endif

