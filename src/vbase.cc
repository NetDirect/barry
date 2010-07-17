//
// \file	vbase.cc
//		vformat support routines in base class
//

/*
    Copyright (C) 2006-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
//#include "trace.h"
#include "vformat.h"		// comes from opensync, but not a public header yet
#include "tzwrapper.h"
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <sstream>

using namespace std;

namespace Barry { namespace Sync {

//////////////////////////////////////////////////////////////////////////////
// vTimeConverter

std::string vTimeConverter::unix2vtime(const time_t *timestamp)
{
	struct tm split;
	if( !gmtime_r(timestamp, &split) ) {
		ostringstream oss;
		oss << "gmtime_r() failed on time_t of ";
		if( timestamp )
			oss << *timestamp;
		else
			oss << "(null pointer)";
		throw Barry::ConvertError(oss.str());
	}

	return tm_to_iso(&split, true);
}

time_t vTimeConverter::vtime2unix(const char *vtime)
{
	return TzWrapper::iso_mktime(vtime);
}

//
// The following implementation is taken from opensync's
// opensync_time.c implementation with the following copyright
// notices at the top as of July 2010:
//
//  * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
//  * Copyright (C) 2006-2008 Daniel Gollub <gollub@b1-systems.de>
//  * Copyright (C) 2007 Chris Frey <cdfrey@netdirect.ca>
//
// License: LGPL 2.1 or later
//
int vTimeConverter::alarmduration2sec(const char *alarm)
{
        int i, secs, digits = 0;
        int is_digit = 0;
        int sign = 1;   // when ical stamp doesn't start with '-' => seconds after event
        int days = 0, weeks = 0, hours = 0, minutes = 0, seconds = 0;
        int len = strlen(alarm);

        for (i=0; i < len; i++) {

                switch (alarm[i]) {
                case '-':
                        sign = -1; // seconds before event - change the sign
                case 'P':
                case 'T':
                        is_digit = 0;
                        break;
                case 'W':
                        is_digit = 0;
                        weeks = digits;
                        break;
                case 'D':
                        is_digit = 0;
                        days = digits;
                        break;
                case 'H':
                        is_digit = 0;
                        hours = digits;
                        break;
                case 'M':
                        is_digit = 0;
                        minutes = digits;
                        break;
                case 'S':
                        is_digit = 0;
                        seconds = digits;
                        break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                        if (is_digit)
                                break;

                        if (sscanf((char*)(alarm+i),"%d",&digits) == EOF)
                                return -1;

                        is_digit = 1;
                        break;
                }
        }

        secs = (weeks * 7 * 24 * 3600) + (days * 24 * 3600) + (hours * 3600) + (minutes * 60) + seconds;

        secs = secs * sign;     // change sign if the alarm is in seconds before event (leading '-')

        return secs;

}


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

std::string vAttr::GetDecodedValue()
{
	std::string ret;
	GString *value = NULL;

	if( m_attr ) {
		if( b_vformat_attribute_is_single_valued(m_attr) ) {
			value = b_vformat_attribute_get_value_decoded(m_attr);
		}
	}

	if( value )
		ret.assign(value->str, value->len);

	return ret;
}

std::string vAttr::GetParam(const char *name, int nth)
{
	std::string ret;

	if( !m_attr )
		return ret;

	b_VFormatParam *param = b_vformat_attribute_find_param(m_attr, name, 0);
	if( !param )
		return ret;

	const char *value = b_vformat_attribute_param_get_nth_value(param, nth);
	if( value )
		ret = value;

	return ret;
}

/// Does an exhaustive search through the attribute, searching for all
/// param values that exist for the given name, and returns all values
/// in a comma delimited string.
std::string vAttr::GetAllParams(const char *name)
{
	std::string ret;

	if( !m_attr )
		return ret;

	b_VFormatParam *param = 0;
	for( int level = 0;
	     (param = b_vformat_attribute_find_param(m_attr, name, level));
	     level++ )
	{
		const char *value = 0;
		for( int nth = 0;
		     (value = b_vformat_attribute_param_get_nth_value(param, nth));
		     nth++ )
		{
			if( ret.size() )
				ret += ",";
			ret += value;
		}
	}

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
		m_format = 0;
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
//	Trace trace("vBase::NewAttr");

//	trace.logf("creating valueless attr: %s", name);

	vAttrPtr attr(b_vformat_attribute_new(NULL, name));
	if( !attr.Get() )
		throw Barry::ConvertError("resource error allocating vformat attribute");
	return attr;
}

vAttrPtr vBase::NewAttr(const char *name, const char *value)
{
//	Trace trace("vBase::NewAttr");

/*
some vCard values are positional (like name), so blank should be allowed...

	if( strlen(value) == 0 ) {
		trace.logf("attribute '%s' contains no data, skipping", name);
		return vAttrPtr();
	}
*/

//	trace.logf("creating attr: %s, %s", name, value);

	vAttrPtr attr(b_vformat_attribute_new(NULL, name));
	if( !attr.Get() )
		throw ConvertError("resource error allocating vformat attribute");

	b_vformat_attribute_add_value(attr.Get(), value);
	return attr;
}

void vBase::AddAttr(vAttrPtr attr)
{
//	Trace trace("vBase::AddAttr");

	if( !attr.Get() ) {
//		trace.log("attribute contains no data, skipping");
		return;
	}

	b_vformat_add_attribute(m_format, attr.Extract());
}

void vBase::AddValue(vAttrPtr &attr, const char *value)
{
//	Trace trace("vBase::AddValue");
	if( !attr.Get() ) {
//		trace.log("attribute pointer contains no data, skipping");
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

void vBase::AddEncodedValue(vAttrPtr &attr, b_VFormatEncoding encoding, const char *value, int len)
{
//	Trace trace("vBase::AddValue");
	if( !attr.Get() ) {
//		trace.log("attribute pointer contains no data, skipping");
		return;
	}

	attr.Get()->encoding = encoding;
	attr.Get()->encoding_set = TRUE;

	b_vformat_attribute_add_value_decoded(attr.Get(), value, len);
}

void vBase::AddParam(vAttrPtr &attr, const char *name, const char *value)
{
//	Trace trace("vBase::AddParam");

	if( !attr.Get() ) {
//		trace.log("attribute pointer contains no data, skipping");
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
//	Trace trace("vBase::GetAttr");
//	trace.logf("getting attr: %s", attrname);

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

//	trace.logf("attr value: %s", ret.c_str());
	return ret;
}

std::vector<std::string> vBase::GetValueVector(const char *attrname, const char *block)
{
//	Trace trace("vBase::GetValueVector");
//	trace.logf("getting value vector for: %s", attrname);

	std::vector<std::string> ret;
	const char *value = 0;
	bool needs_freeing = false;

	b_VFormatAttribute *attr = b_vformat_find_attribute(m_format, attrname, 0, block);
	if( attr ) {
		if( b_vformat_attribute_is_single_valued(attr) ) {
			value = b_vformat_attribute_get_value(attr);
			needs_freeing = true;
		} else {
			// nasty, but avoids tweaking vformat.
			int idx = 0;
			do {
				value = b_vformat_attribute_get_nth_value(attr, idx++);
				if( value ) {
					ret.push_back(value);
				}
			} while( value );
		}
	}

	if( needs_freeing )
		g_free((char *)value);

	return ret;
}

vAttr vBase::GetAttrObj(const char *attrname, int nth, const char *block)
{
//	Trace trace("vBase::GetAttrObj");
//	trace.logf("getting attr: %s", attrname);

	return vAttr(b_vformat_find_attribute(m_format, attrname, nth, block));
}

std::vector<std::string> vBase::Tokenize(const std::string& str, const char delim)
{
	std::vector<std::string> tokens;
	std::string::size_type delimPos = 0, tokenPos = 0, pos = 0;

	if( str.length() < 1 ) {
		return tokens;
	}

	while( 1 ) {
		delimPos = str.find_first_of(delim, pos);
		tokenPos = str.find_first_not_of(delim, pos);

		if( std::string::npos != delimPos ) {
			if( std::string::npos != tokenPos ) {
				if( tokenPos < delimPos ) {
					tokens.push_back(str.substr(pos, delimPos-pos));
				} else {
					tokens.push_back("");
				}
			} else {
				tokens.push_back("");
			}
			pos = delimPos + 1;
		} else {
			if( std::string::npos != tokenPos ){
				tokens.push_back(str.substr(pos));
			} else {
				tokens.push_back("");
			}
			break;
		}
	}
	return tokens;
}

std::string vBase::ToStringList(const std::vector<std::string> &list, const char delim)
{
	std::string str;
	for( unsigned int idx = 0; idx < list.size(); idx++ ) {
		if( idx ) {
			str += delim;
		}
		str += list[idx];
	}
	return str;
}

}} // namespace Barry::Sync

