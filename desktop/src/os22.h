///
/// \file	os22.h
///		Wrapper class for opensync 0.22 syncing behaviour
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_OS22_H__
#define __BARRYDESKTOP_OS22_H__

#include "dlopen.h"
#include "osbase.h"

namespace OpenSync {

class OpenSync22Private;

class OpenSync22 : public DlOpen, public OpenSync::API
{
	static bool symbols_loaded;

	OpenSync22Private *m_priv;

protected:
	void SetupEnvironment(OpenSync22Private *p);

public:
	OpenSync22();
	~OpenSync22();

	static bool SymbolsLoaded() { return symbols_loaded; }

	//
	// Virtual API overrides
	//

	// General engine information
	const char* GetVersion() const;
	void GetPluginNames(string_list_type &plugins);
	void GetFormats(format_list_type &formats);

	// Information about configured groups
	void GetGroupNames(string_list_type &groups);
	void GetMembers(const std::string &group_name,
		member_list_type &members);

	// Group configuration
	void AddGroup(const std::string &group_name);
	void DeleteGroup(const std::string &group_name);

	// Member configuration

	// Syncing
};

} // namespace OpenSync

#endif

