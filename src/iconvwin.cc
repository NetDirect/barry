///
/// \file	iconvwin.cc
///		iconv wrapper class for Windows
///

/*
    Copyright (C) 2008-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <errno.h>
#include <string>

using namespace std;

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// IConvHandlePrivate class
class IConvHandlePrivate
{
public:
};

//////////////////////////////////////////////////////////////////////////////
// IConvHandle class

IConvHandle::IConvHandle(const char *fromcode,
			 const char *tocode,
			 bool throw_on_conv_err)
	: m_priv( new IConvHandlePrivate )
	, m_throw_on_conv_err(throw_on_conv_err)
{
}

IConvHandle::IConvHandle(const char *fromcode,
			 const IConverter &ic,
			 bool throw_on_conv_err)
	: m_priv( new IConvHandlePrivate )
	, m_throw_on_conv_err(throw_on_conv_err)
{
}

IConvHandle::IConvHandle(const IConverter &ic,
			 const char *tocode,
			 bool throw_on_conv_err)
	: m_priv( new IConvHandlePrivate )
	, m_throw_on_conv_err(throw_on_conv_err)
{
}

IConvHandle::~IConvHandle()
{
}

std::string IConvHandle::Convert(Data &tmp, const std::string &str) const
{
	// FIXME - need to add Windows support
	return str;
}


//////////////////////////////////////////////////////////////////////////////
// IConvHandle class

IConverter::IConverter(const char *tocode, bool throw_on_conv_err)
	: m_from(BLACKBERRY_CHARSET, tocode, throw_on_conv_err)
	, m_to(tocode, BLACKBERRY_CHARSET, throw_on_conv_err)
	, m_tocode(tocode)
{
}

IConverter::~IConverter()
{
}

std::string IConverter::FromBB(const std::string &str) const
{
	return m_from.Convert(m_buffer, str);
}

std::string IConverter::ToBB(const std::string &str) const
{
	return m_to.Convert(m_buffer, str);
}

std::string IConverter::Convert(const IConvHandle &custom, const std::string &str) const
{
	return custom.Convert(m_buffer, str);
}

} // namespace Barry

