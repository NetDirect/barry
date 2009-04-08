///
/// \file	iconv.cc
///		iconv wrapper class, and pluggable interface for records
///

/*
    Copyright (C) 2008-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "iconv.h"
#include "common.h"
#include "error.h"
#include "config.h"
#include <errno.h>

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// IConvHandle class

IConvHandle::IConvHandle(const char *fromcode, const char *tocode)
{
	m_handle = iconv_open(tocode, fromcode);
	if( m_handle == (iconv_t)(-1) ) {
		throw ErrnoError(std::string("iconv_open failed: from ") + fromcode + " to " + tocode, errno);
	}
}

IConvHandle::IConvHandle(const char *fromcode, const IConverter &ic)
{
	m_handle = iconv_open(ic.m_tocode.c_str(), fromcode);
	if( m_handle == (iconv_t)(-1) ) {
		throw ErrnoError(std::string("iconv_open failed: from ") + fromcode + " to " + ic.m_tocode, errno);
	}
}

IConvHandle::IConvHandle(const IConverter &ic, const char *tocode)
{
	m_handle = iconv_open(tocode, ic.m_tocode.c_str());
	if( m_handle == (iconv_t)(-1) ) {
		throw ErrnoError(std::string("iconv_open failed: from ") + ic.m_tocode + " to " + tocode, errno);
	}
}

IConvHandle::~IConvHandle()
{
	iconv_close(m_handle);
}


//////////////////////////////////////////////////////////////////////////////
// IConvHandle class

IConverter::IConverter(const char *tocode, bool throw_on_conv_err)
	: m_from(BLACKBERRY_CHARSET, tocode)
	, m_to(tocode, BLACKBERRY_CHARSET)
	, m_tocode(tocode)
	, m_throw_on_conv_err(throw_on_conv_err)
{
}

IConverter::~IConverter()
{
}

std::string IConverter::Convert(iconv_t cd, const std::string &str) const
{
	size_t target = str.size() * 2;
	char *out = 0, *outstart = 0;
	size_t outbytesleft = 0;
	std::string ret;

	// this loop is for the very odd case that the output string
	// needs more than twice the input size
	for( int tries = 0; ; tries++ ) {

		const char *in = str.data();
		size_t inbytesleft = str.size();
		out = outstart = (char*) m_buffer.GetBuffer(target);
		outbytesleft = m_buffer.GetBufSize();

		iconv(cd, NULL, NULL, NULL, NULL);	// reset cd's state
		size_t status = iconv(cd, (ICONV_CONST char**) &in, &inbytesleft, &out, &outbytesleft);

		if( status == (size_t)(-1) ) {
			if( errno == E2BIG && tries < 2 ) {
				target += inbytesleft * 2;
				// try again with more memory...
				continue;
			}

			// should never happen :-)
			// but if it does, and we get here, check
			// whether the user wants to be notified by
			// exception... if not, just fall through and
			// store as much converted data as possible
			if( m_throw_on_conv_err ) {
				throw ErrnoError("iconv failed", errno);
			}
		}
		else {
			// success
			break;
		}
	}

	// store any available converted data
	ret.assign(outstart, out - outstart);
	return ret;
}

std::string IConverter::FromBB(const std::string &str) const
{
	return Convert(m_from.m_handle, str);
}

std::string IConverter::ToBB(const std::string &str) const
{
	return Convert(m_to.m_handle, str);
}

std::string IConverter::Convert(const IConvHandle &custom, const std::string &str) const
{
	return Convert(custom.m_handle, str);
}

} // namespace Barry

