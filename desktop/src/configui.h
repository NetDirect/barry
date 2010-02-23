///
/// \file	configui.h
///		Base class for plugin config user interfaces
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

#ifndef __BARRY_CONFIGUI_H__
#define __BARRY_CONFIGUI_H__

#include <wx/wx.h>
#include <memory>
#include <tr1/memory>
#include <string>
#include "osconfig.h"

//
// ConfigUI
//
/// Base class for plugin config user interfaces
///
/// To use, call the static factory function to create the config UI
/// object for a given App.  Then call Configure(), which will block
/// and may do a number of configuration steps to let the user
/// configure the App.  If Configure() returns true, then call
/// GetPlugin() to retrieve the fully configured plugin.
///
class ConfigUI
{
public:
	typedef OpenSync::Config::Group::plugin_ptr		plugin_ptr;
	typedef std::auto_ptr<ConfigUI>				configui_ptr;
	typedef configui_ptr					ptr;

public:
	virtual ~ConfigUI() {}

	virtual bool Configure(wxWindow *parent) = 0;
	virtual plugin_ptr GetPlugin() = 0;

	static configui_ptr CreateConfigUI(const std::string &appname);
};

#endif

