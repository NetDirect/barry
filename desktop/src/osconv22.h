///
/// \file	osconv22.h
///		Converter class for opensync 0.22 plugins
///

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_OSCONV22_H__
#define __BARRY_OSCONV22_H__

#include "osbase.h"

namespace OpenSync {

class Converter22 : public Converter
{
	OpenSync::API &m_api;

protected:
	std::string GrabField(const std::string &cfg, const std::string &name);

public:
	Converter22(OpenSync::API &api);

	virtual bool IsPluginSupported(const std::string &plugin_name,
		std::string *appname = 0) const;
	virtual plugin_ptr CreateAndLoadPlugin(const Member &member);

	virtual std::string GetPluginName(const Config::Barry &) const;
	virtual std::string GetPluginName(const Config::Evolution &) const;
	virtual std::string GetPluginName(const Config::Google &) const;
	virtual std::string GetPluginName(const Config::Unsupported &) const;

	virtual bool IsConfigured(const Config::Barry &) const;
	virtual bool IsConfigured(const Config::Evolution &) const;
	virtual bool IsConfigured(const Config::Google &) const;
	virtual bool IsConfigured(const Config::Unsupported &) const;

	virtual void Load(Config::Barry &config, const Member &member);
	virtual void Load(Config::Evolution &config, const Member &member);
	virtual void Load(Config::Google &config, const Member &member);
	virtual void Load(Config::Unsupported &config, const Member &member);

	virtual void Save(const Config::Barry &config, const std::string &group_name);
	virtual void Save(const Config::Evolution &config, const std::string &group_name);
	virtual void Save(const Config::Google &config, const std::string &group_name);
	virtual void Save(const Config::Unsupported &config, const std::string &group_name);
};

} // namespace OpenSync

#endif

