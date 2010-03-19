///
/// \file	CUI_Evolution.h
///		ConfigUI derived class to configure the Evolution App
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

#ifndef __BARRY_CUI_EVOLUTION_H__
#define __BARRY_CUI_EVOLUTION_H__

#include "configui.h"
#include "osconfig.h"
#include <memory>

class wxWindow;

namespace AppConfig {

class Evolution : public ConfigUI
{
	OpenSync::Config::Evolution *m_evolution;
	plugin_ptr m_container;			// merely holds m_evolution

	// convenience pointers
	wxWindow *m_parent;

protected:
	bool AutoDetect();
	bool InitialRun();

public:
	Evolution();

	// virtual overrides (ConfigUI)
	virtual std::string AppName() const;
	virtual bool Configure(wxWindow *parent);
	virtual plugin_ptr GetPlugin();
	virtual bool RunApp(wxWindow *parent);
	virtual void PreSyncAppInit();

	// static utility functions
	static long ForceShutdown();
};

}

#endif

