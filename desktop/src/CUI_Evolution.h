///
/// \file	CUI_Evolution.h
///		ConfigUI derived class to configure the Evolution App
///

/*
    Copyright (C) 2009-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

class EvolutionPtrBase : public ConfigUI
{
private:
	OpenSync::Config::Evolution *m_evolution;

protected:
	plugin_ptr m_container;			// merely holds m_evolution

protected:
	EvolutionPtrBase();

	// by default, creates Evolution object... override for others
	virtual void AcquirePlugin(plugin_ptr old_plugin);
	virtual OpenSync::Config::Evolution* GetEvolutionPtr();
	virtual void Clear();

public:
	virtual plugin_ptr GetPlugin();
};

class Evolution : public EvolutionPtrBase
{
private:
	// convenience pointers
	wxWindow *m_parent;

protected:
	bool InitialRun();

public:
	Evolution();

	// virtual overrides (ConfigUI)
	virtual std::string AppName() const;
	virtual bool Configure(wxWindow *parent, plugin_ptr old_plugin);
	virtual bool RunApp(wxWindow *parent);
	virtual void PreSyncAppInit();
	virtual bool ZapData(wxWindow *parent, plugin_ptr plugin,
		OpenSync::API *engine);

	// static utility functions
	static long ForceShutdown();
};

class Evolution3 : public Evolution
{
private:
	OpenSync::Config::Evolution3 *m_evolution3;

protected:
	virtual void AcquirePlugin(plugin_ptr old_plugin);
	virtual OpenSync::Config::Evolution* GetEvolutionPtr();
	virtual void Clear();

public:
	Evolution3();

	virtual plugin_ptr GetPlugin();
};

}

#endif

