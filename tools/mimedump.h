///
/// \file	mimedump.h
///		Overloaded templates for handling Mime output
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

#ifndef __BARRY_TOOLS_MIMEDUMP_H__
#define __BARRY_TOOLS_MIMEDUMP_H__

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
class MimeDump<Barry::Memo>
{
public:
	static void Dump(std::ostream &os, const Barry::Memo &rec)
	{
		Barry::Sync::vJournal vjournal;
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

#endif

