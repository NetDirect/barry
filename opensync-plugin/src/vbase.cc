//
// \file	vbase.cc
//		vformat support routines in base class
//

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

#include "vbase.h"
#include "trace.h"
#include "vformat.h"		// comes from opensync, but not a public header yet
#include <stdint.h>
#include <glib.h>
#include <sstream>


//////////////////////////////////////////////////////////////////////////////
// vAttr

std::string vAttr::GetName()
{
	std::string ret;

	if( !m_attr )
		return ret;

	const char *name = b_vformat_attribute_get_name(m_attr);
	if( name )
		ret = name;
	return ret;
}

std::string vAttr::GetValue(int nth)
{
	std::string ret;
	const char *value = 0;

	if( m_attr ) {
		if( b_vformat_attribute_is_single_valued(m_attr) ) {
			if( nth == 0 )
				value = b_vformat_attribute_get_value(m_attr);
		}
		else {
			value = b_vformat_attribute_get_nth_value(m_attr, nth);
		}
	}

	if( value )
		ret = value;

	return ret;
}

std::string vAttr::GetParam(const char *name, int nth)
{
	std::string ret;

	if( !m_attr )
		return ret;

	b_VFormatParam *param = b_vformat_attribute_find_param(m_attr, name);
	if( !param )
		return ret;

	const char *value = b_vformat_attribute_param_get_nth_value(param, nth);
	if( value )
		ret = value;

	return ret;
}


//////////////////////////////////////////////////////////////////////////////
// vCalendar

vBase::vBase()
	: m_format(0)
{
}

vBase::~vBase()
{
	if( m_format ) {
		b_vformat_free(m_format);
	}
}

void vBase::SetFormat(b_VFormat *format)
{
	if( m_format ) {
		b_vformat_free(m_format);
		m_format = 0;
	}
	m_format = format;
}

void vBase::Clear()
{
	if( m_format ) {
		b_vformat_free(m_format);
		m_format = 0;
	}
}

vAttrPtr vBase::NewAttr(const char *name)
{
	Trace trace("vBase::NewAttr");

	trace.logf("creating valueless attr: %s", name);

	vAttrPtr attr(b_vformat_attribute_new(NULL, name));
	if( !attr.Get() )
		throw ConvertError("resource error allocating vformat attribute");
	return attr;
}

vAttrPtr vBase::NewAttr(const char *name, const char *value)
{
	Trace trace("vBase::NewAttr");

/*
some vCard values are positional (like name), so blank should be allowed...

	if( strlen(value) == 0 ) {
		trace.logf("attribute '%s' contains no data, skipping", name);
		return vAttrPtr();
	}
*/

	trace.logf("creating attr: %s, %s", name, value);

	vAttrPtr attr(b_vformat_attribute_new(NULL, name));
	if( !attr.Get() )
		throw ConvertError("resource error allocating vformat attribute");

	b_vformat_attribute_add_value(attr.Get(), value);
	return attr;
}

void vBase::AddAttr(vAttrPtr attr)
{
	Trace trace("vBase::AddAttr");

	if( !attr.Get() ) {
		trace.log("attribute contains no data, skipping");
		return;
	}

	b_vformat_add_attribute(m_format, attr.Extract());
}

void vBase::AddValue(vAttrPtr &attr, const char *value)
{
	Trace trace("vBase::AddValue");
	if( !attr.Get() ) {
		trace.log("attribute pointer contains no data, skipping");
		return;
	}
/*
	if( strlen(value) == 0 ) {
		trace.log("attribute value is empty, skipping");
		return;
	}
*/
	b_vformat_attribute_add_value(attr.Get(), value);
}

void vBase::AddParam(vAttrPtr &attr, const char *name, const char *value)
{
	Trace trace("vBase::AddParam");

	if( !attr.Get() ) {
		trace.log("attribute pointer contains no data, skipping");
		return;
	}
/*
	if( strlen(value) == 0 ) {
		trace.log("parameter value is empty, skipping");
		return;
	}
*/

	b_VFormatParam *pParam = b_vformat_attribute_param_new(name);
	b_vformat_attribute_param_add_value(pParam, value);
	b_vformat_attribute_add_param(attr.Get(), pParam);
}

std::string vBase::GetAttr(const char *attrname, const char *block)
{
	Trace trace("vBase::GetAttr");
	trace.logf("getting attr: %s", attrname);

	std::string ret;
	const char *value = 0;

	bool needs_freeing = false;

	b_VFormatAttribute *attr = b_vformat_find_attribute(m_format, attrname, 0, block);
	if( attr ) {
		if( b_vformat_attribute_is_single_valued(attr) ) {
			value = b_vformat_attribute_get_value(attr);
			needs_freeing = true;
		}
		else {
			// FIXME, this is hardcoded
			value = b_vformat_attribute_get_nth_value(attr, 0);
		}
	}

	if( value )
		ret = value;

	if( needs_freeing )
		g_free((char *)value);

	trace.logf("attr value: %s", ret.c_str());
	return ret;
}

vAttr vBase::GetAttrObj(const char *attrname, int nth, const char *block)
{
	Trace trace("vBase::GetAttrObj");
	trace.logf("getting attr: %s", attrname);

	return vAttr(b_vformat_find_attribute(m_format, attrname, nth, block));
}

