///
/// \file	osbase.h
///		Base API class for OpenSync interaction.
///		This API will operate both 0.22 and 0.4x
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

#ifndef __BARRYDESKTOP_OSBASE_H__
#define __BARRYDESKTOP_OSBASE_H__

#include <vector>
#include <string>
#include <iosfwd>

namespace OpenSync {

struct GroupMember
{
	long id;
	std::string friendly_name;	// may not always have a name
	std::string plugin_name;
};

struct Format
{
	std::string name;
	std::string object_type;
};

struct FormatSet : public std::vector<Format>
{
	Format* Find(const char *name);
};

typedef std::vector<std::string>		string_list_type;
typedef std::vector<GroupMember>		member_list_type;
typedef FormatSet				format_list_type;

std::ostream& operator<< (std::ostream &os, const string_list_type &list);
std::ostream& operator<< (std::ostream &os, const GroupMember &member);
std::ostream& operator<< (std::ostream &os, const member_list_type &list);
std::ostream& operator<< (std::ostream &os, const Format &format);
std::ostream& operator<< (std::ostream &os, const format_list_type &list);


class API
{
public:
public:
	API()
	{
	}

	virtual ~API()
	{
	}

	virtual const char* GetVersion() const = 0;
	virtual void GetPluginNames(string_list_type &plugins) = 0;
	virtual void GetGroupNames(string_list_type &groups) = 0;
	virtual void GetMembers(const std::string &group_name,
		member_list_type &members) = 0;
	virtual void GetFormats(format_list_type &formats) = 0;
};

class APISet : private std::vector<API*>
{
	typedef std::vector<API*>		base_type;

public:
	APISet();
	~APISet();

	void OpenAll();		// throws if not all can be opened
	int OpenAvailable();	// opens only what is available and
				// returns # of APIs successfully loaded.
				// throws if some already loaded

	API* os40();
	API* os22();
};

} // namespace OpenSync

#endif

