///
/// \file	vbase.h
///		Base class for vformat support
///

/*
    Copyright (C) 2006-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "dll.h"
#include "vsmartptr.h"
#include "vformat.h"
#include "error.h"
#include <vector>

namespace Barry {
	class CategoryList;
}

namespace Barry { namespace Sync {

//
// vTimeConverter
//
/// A virtual base class that the plugins may override, to do
/// time related conversions.  Default implementations for these
/// functions are provided, but may be overrided depending on need.
///
/// We do this in a "callback" style, so that it doesn't matter what
/// version of the opensync library we link against, in case the
/// user wishes to use the opensync time functions.
///
class BXEXPORT vTimeConverter
{
public:
	virtual ~vTimeConverter() {}

	/// Convert a time_t into an ISO timestamp string
	/// Throws Barry::ConvertError on error, but these errors
	/// must be rare.
	virtual std::string unix2vtime(const time_t *timestamp);

	/// Convert an ISO timestamp string into a time_t, using
	/// the current system timezone if vtime is not in UTC.
	/// Returns (time_t)-1 on error.
	virtual time_t vtime2unix(const char *vtime);

	/// Convert a VEVENT alarm duration string in the format
	/// of "[+-]P.W.DT.H.M.S" where the periods represent numbers
	/// and each letter besides P and T represent Week, Day,
	/// Hour, Minute, and Second respectively.
	virtual int alarmduration2sec(const char *alarm);
};


typedef Barry::vSmartPtr<b_VFormatAttribute, b_VFormatAttribute, &b_vformat_attribute_free> vAttrPtr;
typedef Barry::vSmartPtr<b_VFormatParam, b_VFormatParam, &b_vformat_attribute_param_free> vParamPtr;
typedef Barry::vSmartPtr<char, void, &g_free> gStringPtr;


//
// vAttr
//
/// Class for reading a b_VFormatAttribute.  Reading does not require
/// memory management, so none is done.
///
class BXEXPORT vAttr
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
class BXEXPORT vBase
{
	// internal data for managing the vformat
	b_VFormat *m_format;

public:
protected:
	vBase();
	explicit vBase(b_VFormat *format);
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

	void AddCategories(const Barry::CategoryList &categories);

	std::string GetAttr(const char *attrname, const char *block = 0);
	std::vector<std::string> GetValueVector(const char *attrname, const char *block = 0);
	vAttr GetAttrObj(const char *attrname, int nth = 0, const char *block = 0);

	std::vector<std::string> Tokenize(const std::string &str, const char delim = ',');
	std::string ToStringList(const std::vector<std::string> &list, const char delim = ',');
};

}} // namespace Barry::Sync

#endif

