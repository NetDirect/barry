///
/// \file	configui.cc
///		Base class for plugin config user interfaces
///

/*
    Copyright (C) 2009-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "configui.h"
#include "osconfig.h"
#include "CUI_Evolution.h"
#include "CUI_Barry.h"
#include "CUI_Google.h"
#include "CUI_KDEPim.h"

// Static factory function
ConfigUI::configui_ptr ConfigUI::CreateConfigUI(const std::string &appname)
{
	ConfigUI::configui_ptr ui;

	if( appname == OpenSync::Config::Barry::AppName() ) {
		ui.reset( new AppConfig::Barry );
	}
	else if( appname == OpenSync::Config::Evolution::AppName() ) {
		ui.reset( new AppConfig::Evolution );
	}
	else if( appname == OpenSync::Config::Google::AppName() ) {
		ui.reset( new AppConfig::Google );
	}
	else if( appname == OpenSync::Config::KDEPim::AppName() ) {
		ui.reset( new AppConfig::KDEPim );
	}

	return ui;
}

//////////////////////////////////////////////////////////////////////////////
// ConfigUI class

ConfigUI::ConfigUI()
	: ExecHelper(0)
{
}

ConfigUI::~ConfigUI()
{
}

