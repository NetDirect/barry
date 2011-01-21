///
/// \file	builder.cc
///		Virtual protocol packet builder wrapper
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "builder.h"
#include <stdexcept>
#include <string.h>

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// DBDataBuilder class


DBDataBuilder::DBDataBuilder(const DBData &orig)
	: m_orig(orig)
{
}

DBDataBuilder::~DBDataBuilder()
{
}

bool DBDataBuilder::BuildRecord(DBData &data, size_t &offset,
				const IConverter *ic)
{
	if( offset == m_orig.GetOffset() ) {
		data = m_orig;
	}
	else {
		// copy the metadata
		data.CopyMeta(m_orig);

		// copy the buffer, to the new offset
		if( m_orig.GetOffset() > m_orig.GetData().GetSize() )
			throw std::logic_error("DBDataBuilder: offset greater than size");
		size_t actual = m_orig.GetData().GetSize() - m_orig.GetOffset();
		size_t total = offset + actual;
		unsigned char *buf = data.UseData().GetBuffer(total);
		memcpy(buf + offset,
			m_orig.GetData().GetData() + m_orig.GetOffset(),
			actual);
		data.UseData().ReleaseBuffer(total);

		// set the new offset
		data.SetOffset(offset);
	}
	return true;
}

bool DBDataBuilder::FetchRecord(DBData &data, const IConverter *ic)
{
	data = m_orig;
	return true;
}

bool DBDataBuilder::EndOfFile() const
{
	return true;
}

} // namespace Barry


