///
/// \file	null-os40.cc
///		Null wrapper class for with opensync 0.4x is not available
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

#include "os40.h"
#include "osconv40.h"

namespace OpenSync {

/////////////////////////////////////////////////////////////////////////////
// OpenSync40 - public members

OpenSync40::OpenSync40()
{
	throw std::runtime_error("OpenSync 0.4x support was not compiled in.");
}

OpenSync40::~OpenSync40()
{
}

/////////////////////////////////////////////////////////////////////////////
// Null implementations

const char* OpenSync40::GetVersion() const
{
	return 0;
}

void OpenSync40::GetPluginNames(string_list_type &plugins)
{
}

void OpenSync40::GetFormats(format_list_type &formats)
{
}

void OpenSync40::GetGroupNames(string_list_type &groups)
{
}

void OpenSync40::GetMembers(const std::string &group_name,
				member_list_type &members)
{
}

void OpenSync40::AddGroup(const std::string &group_name)
{
}

void OpenSync40::DeleteGroup(const std::string &group_name)
{
}

Converter& OpenSync40::GetConverter()
{
	throw std::logic_error("Not supported on this system.");
}

long OpenSync40::AddMember(const std::string &group_name,
				const std::string &plugin_name,
				const std::string &member_name)
{
	return 0;
}

void OpenSync40::DeleteMember(const std::string &group_name, long member_id)
{
}

void OpenSync40::DeleteMember(const std::string &group_name,
				const std::string &plugin_name)
{
}

bool OpenSync40::IsConfigurable(const std::string &group_name,
				long member_id)
{
	return false;
}

std::string OpenSync40::GetConfiguration(const std::string &group_name,
					long member_id)
{
	return "";
}

void OpenSync40::SetConfiguration(const std::string &group_name,
			long member_id, const std::string &config_data)
{
}

void OpenSync40::Discover(const std::string &group_name)
{
}

void OpenSync40::Sync(const std::string &group_name,
			SyncStatus &status_callback)
{
}

//////////////////////////////////////////////////////////////////////////////
// Null Converter40

Converter40::Converter40(OpenSync::OpenSync40 &api)
	: m_api(api)
{
}

Converter::plugin_ptr Converter40::CreateAndLoadPlugin(const Member &member)
{
	return Converter::plugin_ptr();
}

std::string Converter40::GetPluginName(const Config::Barry &)
{
	throw std::logic_error("Not supported on this system.");
}

std::string Converter40::GetPluginName(const Config::Evolution &)
{
	throw std::logic_error("Not supported on this system.");
}

std::string Converter40::GetPluginName(const Config::Unsupported &)
{
	throw std::logic_error("Not supported on this system.");
}


void Converter40::Load(Config::Barry &config, const Member &member)
{
	throw std::logic_error("Not supported on this system.");
}

void Converter40::Load(Config::Evolution &config, const Member &member)
{
	throw std::logic_error("Not supported on this system.");
}

void Converter40::Load(Config::Unsupported &config, const Member &member)
{
	throw std::logic_error("Not supported on this system.");
}

void Converter40::Save(const Config::Barry &config, const std::string &group_name)
{
	throw std::logic_error("Not supported on this system.");
}

void Converter40::Save(const Config::Evolution &config, const std::string &group_name)
{
	throw std::logic_error("Not supported on this system.");
}

void Converter40::Save(const Config::Unsupported &config, const std::string &group_name)
{
	throw std::logic_error("Not supported on this system.");
}

}

