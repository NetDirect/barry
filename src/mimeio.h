///
/// \file	mimeio.h
///		Storage, parser, builder classes for MIME objects
///		(vcard, vevent, vtodo, vjournal)
///

/*
    Copyright (C) 2010-2013, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "vcard.h"
#include "vevent.h"
#include "vjournal.h"
#include "vtodo.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace Barry {

class Contact;
class Calendar;
class CalendarAll;
class Memo;
class Task;

//
// Template classes to write MIME data to stream, from record.
//

template <class Record>
class MimeDump
{
public:
	static void Dump(std::ostream &os, const Record &rec)
	{
		os << rec << std::endl;
	}

	static bool Supported() { return false; }
};

template <>
class MimeDump<Barry::Contact>
{
public:
	static void Dump(std::ostream &os, const Barry::Contact &rec)
	{
		Barry::Sync::vCard vcard;
		os << vcard.ToVCard(rec) << std::endl;
	}

	static bool Supported() { return true; }
};

template <>
class MimeDump<Barry::Calendar>
{
public:
	static void Dump(std::ostream &os, const Barry::Calendar &rec)
	{
		Barry::Sync::vTimeConverter vtc;
		Barry::Sync::vCalendar vcal(vtc);
		os << vcal.ToVCal(rec) << std::endl;
	}

	static bool Supported() { return true; }
};

template <>
class MimeDump<Barry::CalendarAll>
{
public:
	static void Dump(std::ostream &os, const Barry::CalendarAll &rec)
	{
		Barry::Sync::vTimeConverter vtc;
		Barry::Sync::vCalendar vcal(vtc);
		os << vcal.ToVCal(rec) << std::endl;
	}

	static bool Supported() { return true; }
};

template <>
class MimeDump<Barry::Memo>
{
public:
	static void Dump(std::ostream &os, const Barry::Memo &rec)
	{
		Barry::Sync::vTimeConverter vtc;
		Barry::Sync::vJournal vjournal(vtc);
		os << vjournal.ToMemo(rec) << std::endl;
	}

	static bool Supported() { return true; }
};

template <>
class MimeDump<Barry::Task>
{
public:
	static void Dump(std::ostream &os, const Barry::Task &rec)
	{
		Barry::Sync::vTimeConverter vtc;
		Barry::Sync::vTodo vtodo(vtc);
		os << vtodo.ToTask(rec) << std::endl;
	}

	static bool Supported() { return true; }
};


//
// Builder class, for reading MIME stream data and loading into
// a DBData record.
//

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

