///
/// \file	EvoSources.h
///		A class that creates a list of Evolution data sources.
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)
    Based on evolution2_sync.h and list_sources.c from evolution opensync plugin
    by Ian Martin
    Copyright (C) 2009, Ian Martin, licensed under LGPL 2.1

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

#ifndef __BARRY_EVO_SOURCES_H__
#define __BARRY_EVO_SOURCES_H__

#include <vector>
#include <string>
#include <memory>
//#include "dlopen.h"

struct EvoSource
{
	std::string m_GroupName;
	std::string m_SourceName;
	std::string m_SourcePath;
};

struct EvoFunctions;

//
// EvoSources
//
/// Instatiating this class does the work of filling the list.  Just
/// probe the data with the member functions as needed.
///
class EvoSources //: public DlOpen
{
public:
	typedef std::vector<EvoSource>		List;

private:
	std::auto_ptr<EvoFunctions> m_funcs;
	bool m_supported;

	List m_addressbook;
	List m_events;
	List m_tasks;
	List m_memos;

protected:
	void LoadBaseSyms();
	bool LoadEbookLib();
	bool LoadEcalLib();
	void GuessPaths();
	void Clear();

public:
	EvoSources();

	/// This is automatically run from inside the constructor.
	/// It can be called again to retry detection with the same instance,
	/// but note that any previous results will be Clear()'d.
	void Detect();

	/// Returns true if this class has built-in support for talking
	/// to the Evolution data server.  False if a dummy wrapper was
	/// built at compile time due to lack of proper libraries.
	bool IsSupported() const;

	/// Returns true if all list are empty
	bool IsEmpty() const;

	/// Returns true if minimum 3 paths are available, and if there is
	/// only 1 path in each list.
	bool IsDefaultable() const;

	const List& GetAddressBook() const { return m_addressbook; }
	const List& GetEvents() const { return m_events; }
	const List& GetTasks() const { return m_tasks; }
	const List& GetMemos() const { return m_memos; }

	static bool PathExists(const std::string &path);
};

#endif

