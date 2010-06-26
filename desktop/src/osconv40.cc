///
/// \file	osconv40.cc
///		Converter class for opensync 0.40 plugins
///

/*
    Copyright (C) 2009-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
		return true;
	}
	else if( plugin_name == PLUGIN_EVOLUTION ) {
		if( appname )
			*appname = Config::Evolution::AppName();
		return true;
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

std::string Converter40::GetPluginName(const Config::Barry &) const
{
	return PLUGIN_BARRY;
}

std::string Converter40::GetPluginName(const Config::Evolution &) const
{
	return PLUGIN_EVOLUTION;
}

std::string Converter40::GetPluginName(const Config::Unsupported &) const
{
	return "unsupported-sync";
}

bool Converter40::IsConfigured(const Config::Barry &config) const
{
	return config.GetPin().valid();
}

bool Converter40::IsConfigured(const Config::Evolution &config) const
{
	// the 40 plugin supports all 4, so check all 4
	return	config.GetAddressPath().size() &&
		config.GetCalendarPath().size() &&
		config.GetTasksPath().size() &&
		config.GetMemosPath().size();
}

bool Converter40::IsConfigured(const Config::Unsupported &) const
{
	return false;
}


void Converter40::Load(Config::Barry &config, const Member &member)
{
	// start with a default setting
	config.DebugMode(false);
	config.SetPassword("");
	config.SetPin(Barry::Pin());

	// read in config for barry-sync
	OS40PluginConfig cfg = m_api.GetConfigurationObj(member.group_name,
							member.id);
	string value = cfg.GetAdvanced("Debug");
	if( value == "1" )
		config.DebugMode(true);

	value = cfg.GetAdvanced("PinCode");
	{
		istringstream iss(value);
		uint32_t pin = 0;
		iss >> hex >> pin;
		config.SetPin(Barry::Pin(pin));
	}

	value = cfg.GetPassword();
	config.SetPassword(value);
}

std::string GrabPath(const std::string &url)
{
	if( url.substr(0, 7) == "file://" )
		return url.substr(7);
	return url;
}

void Converter40::Load(Config::Evolution &config, const Member &member)
{
	OS40PluginConfig cfg = m_api.GetConfigurationObj(member.group_name,
							member.id);

	config.SetAddressPath(GrabPath(cfg.GetResource("contact")->GetUrl()));
	config.SetCalendarPath(GrabPath(cfg.GetResource("event")->GetUrl()));
	config.SetTasksPath(GrabPath(cfg.GetResource("todo")->GetUrl()));
	config.SetMemosPath(GrabPath(cfg.GetResource("note")->GetUrl()));
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

	OS40PluginConfig cfg = m_api.GetConfigurationObj(group_name,
							config.GetMemberId());
	cfg.SetAdvanced("Debug", "Debug mode", OS40PluginConfig::BOOL_TYPE,
		config.IsDebugMode() ? "1" : "0");
	cfg.SetAdvanced("PinCode", "PIN number", OS40PluginConfig::STRING_TYPE,
		config.GetPin().str());

	cfg.SetPassword(config.GetPassword());

	cfg.GetResource("contact")->
		SetName("Contacts").
		Enable(true).
		SetObjFormat("vcard30").
		AddResource();
	cfg.GetResource("event")->
		SetName("Event").
		Enable(true).
		SetObjFormat("vevent20").
		AddResource();
	cfg.GetResource("todo")->
		SetName("Todo").
		Enable(true).
		SetObjFormat("vtodo20").
		AddResource();
	cfg.GetResource("note")->
		SetName("Memo").
		Enable(true).
		SetObjFormat("vjournal").
		AddResource();

	cfg.Save();
}

void Converter40::Save(const Config::Evolution &config, const std::string &group_name)
{
	if( config.GetMemberId() == -1 )
		throw Config::SaveError("Cannot save a plugin with a member_id of -1");

	OS40PluginConfig cfg = m_api.GetConfigurationObj(group_name,
							config.GetMemberId());
	cfg.GetResource("contact")->
		Enable(true).
		SetObjFormat("vcard21", "VCARD_EXTENSION=Evolution").
		SetObjFormat("vcard30", "VCARD_EXTENSION=Evolution").
		SetUrl("file://" + config.GetAddressPath()).
		AddResource();
	cfg.GetResource("event")->
		Enable(true).
		SetObjFormat("vevent20").
		SetUrl("file://" + config.GetCalendarPath()).
		AddResource();
	cfg.GetResource("todo")->
		Enable(true).
		SetObjFormat("vtodo20").
		SetUrl("file://" + config.GetTasksPath()).
		AddResource();
	cfg.GetResource("note")->
		Enable(true).
		SetObjFormat("vjournal").
		SetUrl("file://" + config.GetMemosPath()).
		AddResource();

	cfg.Save();
}

void Converter40::Save(const Config::Unsupported &config, const std::string &group_name)
{
	if( config.GetMemberId() == -1 )
		throw Config::SaveError("Cannot save a plugin with a member_id of -1");

	m_api.SetConfiguration(group_name, config.GetMemberId(),
		config.GetRawConfig());
}

} // namespace OpenSync

