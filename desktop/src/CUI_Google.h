///
/// \file	CUI_Google.h
///		ConfigUI derived class to configure the Google App
///

/*
    Copyright (C) 2010-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_CUI_GOOGLE_H__
#define __BARRY_CUI_GOOGLE_H__

#include "configui.h"
#include "osconfig.h"

class wxWindow;

namespace AppConfig {

class Google : public ConfigUI
{
	OpenSync::Config::Google *m_google;
	plugin_ptr m_container;			// merely holds m_google

	// convenience pointers
	wxWindow *m_parent;

public:
	Google();

	// virtual overrides (ConfigUI)
	virtual std::string AppName() const;
	virtual bool Configure(wxWindow *parent, plugin_ptr old_plugin);
	virtual plugin_ptr GetPlugin();
	virtual bool RunApp(wxWindow *parent);
	virtual void PreSyncAppInit();
	virtual bool ZapData(wxWindow *parent, plugin_ptr plugin,
		OpenSync::API *engine);
};

}

#endif

