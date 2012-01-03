///
/// \file	mimeio.h
///		Storage, parser, builder classes for MIME objects
///		(vcard, vevent, vtodo, vjournal)
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

#ifndef __BARRY_MIMEIO_H__
#define __BARRY_MIMEIO_H__

#include "dll.h"
#include "builder.h"
#include <string>
#include <vector>
#include <memory>
#include <iosfwd>

namespace Barry {

class BXEXPORT MimeBuilder : public Barry::Builder
{
	std::auto_ptr<std::ifstream> m_ifs;
	std::istream &m_is;

public:
	explicit MimeBuilder(const std::string &filename);
	explicit MimeBuilder(std::istream &is);

	bool BuildRecord(DBData &data, size_t &offset, const IConverter *ic);
	bool FetchRecord(DBData &data, const IConverter *ic);
	bool EndOfFile() const;

	// return false at end of file, true if a record was read
	static bool ReadMimeRecord(std::istream &is, std::string &vrec,
				std::vector<std::string> &types);
	// returns true if item is a member of types, doing a
	// case-insensitive compare
	static bool IsMember(const std::string &item,
				const std::vector<std::string> &types);
};

}

#endif

