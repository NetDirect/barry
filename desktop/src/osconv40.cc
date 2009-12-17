///
/// \file	osconv40.cc
///		Converter class for opensync 0.40 plugins
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

#include "osconv40.h"
#include "osconfig.h"
#include <sstream>

using namespace std;

// Supported plugin names
#define PLUGIN_BARRY		"barry-sync"
#define PLUGIN_EVOLUTION	"evo2-sync"
#define PLUGIN_KDEPIM		"kdepim-sync"
#define PLUGIN_FILE		"file-sync"
#define PLUGIN_SUNBIRD		"sunbird-sync"
#define PLUGIN_LDAP		"ldap-sync"

namespace OpenSync {

//////////////////////////////////////////////////////////////////////////////
// Converter40

Converter40::Converter40(OpenSync::OpenSync40 &api)
	: m_api(api)
{
}

bool Converter40::IsPluginSupported(const std::string &plugin_name,
					std::string *appname) const
{
	if( plugin_name == PLUGIN_BARRY ) {
		if( appname )
			*appname = Config::Barry::AppName();
		return false;	// FIXME - not yet implemented
	}
	else if( plugin_name == PLUGIN_EVOLUTION ) {
		if( appname )
			*appname = Config::Evolution::AppName();
		return false;	// FIXME - not yet implemented
	}

	return false;
}

Converter::plugin_ptr Converter40::CreateAndLoadPlugin(const Member &member)
{
	Converter::plugin_ptr ptr;

	// compare plugin name in member with all known plugins that
	// we support... and default to Unsupported if not
	if( member.plugin_name == PLUGIN_BARRY ) {
		ptr.reset( new Config::Barry(this, member) );
	}
	else if( member.plugin_name == PLUGIN_EVOLUTION ) {
		ptr.reset( new Config::Evolution(this, member) );
	}
	// default: Unsupported
	else {
		ptr.reset( new Config::Unsupported(this, member) );
	}

	return ptr;
}

std::string Converter40::GetPluginName(const Config::Barry &)
{
	return PLUGIN_BARRY;
}

std::string Converter40::GetPluginName(const Config::Evolution &)
{
	return PLUGIN_EVOLUTION;
}

std::string Converter40::GetPluginName(const Config::Unsupported &)
{
	return "unsupported-sync";
}


void Converter40::Load(Config::Barry &config, const Member &member)
{
	// start with a default setting
	config.DebugMode(false);
	config.SetPassword("");
	config.SetPin(Barry::Pin());

	throw std::logic_error("Not yet implemented");
}

void Converter40::Load(Config::Evolution &config, const Member &member)
{
	string cfg = m_api.GetConfiguration(member.group_name, member.id);

	throw std::logic_error("Not yet implemented");
}

void Converter40::Load(Config::Unsupported &config, const Member &member)
{
	string cfg = m_api.GetConfiguration(member.group_name, member.id);
	config.SetRawConfig(cfg);
}

void Converter40::Save(const Config::Barry &config, const std::string &group_name)
{
	if( config.GetMemberId() == -1 )
		throw Config::SaveError("Cannot save a plugin with a member_id of -1");

	throw std::logic_error("Not yet implemented");
}

void Converter40::Save(const Config::Evolution &config, const std::string &group_name)
{
	if( config.GetMemberId() == -1 )
		throw Config::SaveError("Cannot save a plugin with a member_id of -1");

	throw std::logic_error("Not yet implemented");
}

void Converter40::Save(const Config::Unsupported &config, const std::string &group_name)
{
	if( config.GetMemberId() == -1 )
		throw Config::SaveError("Cannot save a plugin with a member_id of -1");

	m_api.SetConfiguration(group_name, config.GetMemberId(),
		config.GetRawConfig());
}

} // namespace OpenSync

