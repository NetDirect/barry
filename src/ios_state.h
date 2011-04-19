///
/// \file	ios_state.h
///		RAII class to save and restore iostream state
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_IOS_STATE_H__
#define __BARRY_IOS_STATE_H__

#include <ios>
#include <ostream>

namespace Barry {

//
// ios_format_state
//
/// This class saves the following stream settings:
///	- flags
///	- precision
///	- width
///	- fill character
///
/// and restores them in the destructor.
///
class ios_format_state
{
public:
	typedef std::ios		stream_type;

private:
	stream_type			&m_stream;
	std::ios_base::fmtflags		m_flags;
	std::streamsize			m_precision;
	std::streamsize			m_width;
	stream_type::char_type		m_fill;

public:
	explicit ios_format_state(stream_type &stream)
		: m_stream(stream)
		, m_flags(stream.flags())
		, m_precision(stream.precision())
		, m_width(stream.width())
		, m_fill(stream.fill())
	{
	}

	~ios_format_state()
	{
		restore();
	}

	void restore()
	{
		m_stream.flags(m_flags);
		m_stream.precision(m_precision);
		m_stream.width(m_width);
		m_stream.fill(m_fill);
	}
};

}

#endif

