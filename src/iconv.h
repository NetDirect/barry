///
/// \file	iconv.h
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

#ifndef __BARRY_ICONV_H__
#define __BARRY_ICONV_H__

#include "dll.h"
#include "data.h"
#include <iconv.h>
#include <string>

namespace Barry {

class BXEXPORT IConverter
{
	iconv_t m_from;
	iconv_t m_to;

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
};

} // namespace Barry

#endif

