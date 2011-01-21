///
/// \file	pipe.cc
///		Connector class to join parsers and builders together
///

/*
    Copyright (C) 2010-2011, Net Direct Inc. (http://www.netdirect.ca/)

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
{
}

Pipe::~Pipe()
{
}

bool Pipe::PumpEntry(Parser &parser, const IConverter *ic)
{
	// if false, end of series, so pass that on to the caller
	if( !m_builder.FetchRecord(m_buffer, ic) )
		return false;

	// pass the data into the parser
	parser.ParseRecord(m_buffer, ic);
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

