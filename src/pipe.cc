///
/// \file	pipe.cc
///		Connector class to join parsers and builders together
///

/*
    Copyright (C) 2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "pipe.h"

namespace Barry {

Pipe::Pipe(Builder &builder)
	: m_builder(builder)
	, m_valid_builder(false)
{
	// call it once to load the builder with data,
	// in case the user calls GetBuilder().GetDBName()
	m_valid_builder = m_builder.Retrieve();
}

Pipe::~Pipe()
{
}

bool Pipe::PumpEntry(Parser &parser, const IConverter *ic)
{
	// if the very first call (constructor) returns false, don't
	// rely on this builder
	if( !m_valid_builder )
		return false;

	// if false, end of series, so pass that on to the caller
	if( !m_builder.Retrieve() )
		return false;

	size_t offset = 0;
	m_builder.BuildHeader(m_buffer, offset);
	m_builder.BuildFields(m_buffer, offset, ic);

//	size_t total_size = m_buffer.GetSize();
	offset = 0;

	parser.Clear();
	parser.SetIds(m_builder.GetDBName(),
		m_builder.GetRecType(), m_builder.GetUniqueId());
	parser.ParseHeader(m_buffer, offset);
	parser.ParseFields(m_buffer, offset, ic);

	parser.Store();
	m_builder.BuildDone();
	return true;
}

/// Reads all items from builder, feeding them into the parser,
/// until the builder's Retrieve() signals the end of the series.
void Pipe::PumpSeries(Parser &parser, const IConverter *ic)
{
	while( PumpEntry(parser, ic) )
		;
}

/// Reads all series from the builder, feeding them into the parser,
/// until the builder's EndOfFile() is true.
void Pipe::PumpFile(Parser &parser, const IConverter *ic)
{
	while( !m_builder.EndOfFile() ) {
		PumpSeries(parser, ic);
	}
}

} // namespace Barry

