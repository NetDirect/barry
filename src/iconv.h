///
/// \file	iconv.h
///		iconv wrapper class, and pluggable interface for records
///

/*
    Copyright (C) 2008-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_ICONV_H__
#define __BARRY_ICONV_H__

#include "dll.h"
#include "data.h"
#include <iconv.h>
#include <string>

namespace Barry {

class IConverter;

//
// IConvHandle class
//
/// Wrapper class for a two-way iconv_t handle pair.  Automatically
/// handles closing in the destructor.
//
class BXEXPORT IConvHandle
{
	friend class IConverter;

	iconv_t m_handle;

private:
	// private constructor, used only by IConverter
	IConvHandle(const char *fromcode, const char *tocode);

public:
	// custom conversions from any to IConverter's 'tocode'
	IConvHandle(const char *fromcode, const IConverter &ic);
	// custom conversions from IConverter's 'tocode' to any
	IConvHandle(const IConverter &ic, const char *tocode);
	~IConvHandle();
};

//
// IConverter
//
/// Main charset conversion class, primarily focused on converting
/// between the Blackberry charset and an application-specified one.
/// Additional conversions are possible through custom IConvHandle,
/// but the goal of this class design is to deal with _one_
/// application defined charset, and provide a means to convert
/// to/from that charset to/from any other charset needed by
/// the Blackberry.
///
/// By default, this class assumes the Blackberry's charset is
/// WINDOWS-1252, but some data, such as SMS message bodies, can have
/// custom charsets as specified by the records.  To convert from
/// such a custom charset, use:
///
///      // application sets up IConverter
///      IConverter ic("UTF-8");
///
///      // somewhere in the library, needing to convert
///      // from UCS2 to whatever the application selected
///      IConvHandle ucs2("UCS2", ic);
///      application_string = ic.Convert(ucs2, ucs2_string_data);
///
///      // and to convert back...
///      IConvHandle ucs2_reverse(ic, "UCS2");
///      ucs2_string = ic.Convert(ucs2_reverse, application_string_data);
///
class BXEXPORT IConverter
{
	friend class IConvHandle;

	IConvHandle m_from;
	IConvHandle m_to;
	std::string m_tocode;

	// internal buffer for fast conversions
	mutable Data m_buffer;

	bool m_throw_on_conv_err;

private:
	std::string Convert(iconv_t cd, const std::string &str) const;

public:
	/// Always throws ErrnoError if unable to open iconv.
	/// If throw_on_conv_err is true, then string conversion operations
	/// that fail will also throw ErrnoError.
	explicit IConverter(const char *tocode = "UTF-8", bool throw_on_conv_err = false);
	~IConverter();

	std::string FromBB(const std::string &str) const;
	std::string ToBB(const std::string &str) const;

	// Custom override functions, meant for converting between
	// non-BLACKBERRY_CHARSET charsets and the tocode set by the
	// IConverter constructor
	std::string Convert(const IConvHandle &custom, const std::string &str) const;
};

} // namespace Barry

#endif

