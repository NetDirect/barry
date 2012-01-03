///
/// \file	pipe.h
///		Connector class to join parsers and builders together
///

/*
    Copyright (C) 2010-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_PIPE_H__
#define __BARRY_PIPE_H__

#include "dll.h"
#include "builder.h"
#include "parser.h"

namespace Barry {

//
// class Pipe
//
/// Reads data from a builder and feeds it into a parser.
///
class BXEXPORT Pipe
{
	Builder &m_builder;

	DBData m_buffer;

public:
	explicit Pipe(Builder &builder);
	~Pipe();

	/// Access the builder... mostly useful for finding out
	/// the database name for the next series, if using multiple
	/// parser objects.
	const Builder& GetBuilder() const { return m_builder; }

	/// Reads one item from the builder and feeds it into the parser
	/// and returns the builder Retrieve status.
	bool PumpEntry(Parser &parser, const IConverter *ic = 0);

	/// Reads all items from builder, feeding them into the parser,
	/// until the builder's Retrieve() signals the end of the series.
	void PumpSeries(Parser &parser, const IConverter *ic = 0);

	/// Reads all series from the builder, feeding them into the parser,
	/// until the builder's EndOfFile() is true.
	void PumpFile(Parser &parser, const IConverter *ic = 0);
};

} // namespace Barry

#endif

