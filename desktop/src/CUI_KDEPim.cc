///
/// \file	CUI_KDEPim.cc
///		ConfigUI derived class to configure the KDEPim App
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

#include "CUI_KDEPim.h"
#include "os22.h"			// only for the dynamic_cast
#include <wx/wx.h>
#include <wx/process.h>

namespace AppConfig {

//////////////////////////////////////////////////////////////////////////////
// KDEPim config UI class

KDEPim::KDEPim()
	: m_kdepim(0)
	, m_parent(0)
{
}

std::string KDEPim::AppName() const
{
	return OpenSync::Config::KDEPim::AppName();
}

bool KDEPim::Configure(wxWindow *parent, plugin_ptr old_plugin)
{
	m_parent = parent;

	// create our plugin config
	m_kdepim = dynamic_cast<OpenSync::Config::KDEPim*> (old_plugin.get());
	if( m_kdepim ) {
		m_kdepim = m_kdepim->Clone();
	}
	else {
		m_kdepim = new OpenSync::Config::KDEPim;
	}
	m_container.reset( m_kdepim );

	// tell the user all went well
	wxMessageBox(_T("KDEPim needs no configuration."), _T("KDEPim Config"), wxOK | wxICON_INFORMATION, m_parent);
	return true;
}

ConfigUI::plugin_ptr KDEPim::GetPlugin()
{
	m_kdepim = 0;
	return m_container;
}

bool KDEPim::RunApp(wxWindow *parent)
{
	return Run(parent, AppName(), _T("kontact"));
}

void KDEPim::PreSyncAppInit()
{
}

bool KDEPim::ZapData(wxWindow *parent,
			plugin_ptr plugin,
			OpenSync::API *engine)
{
	m_parent = parent;

	if( IsAppRunning() ) {
		wxMessageBox(_T("Kontact already running."),
			_T("No Biscuit"), wxOK | wxICON_INFORMATION,
			m_parent);
		return false;
	}

	// tell the user what to do
	wxString msg;
	if( dynamic_cast<OpenSync::OpenSync22*>(engine) ) {
		msg = _T(
		"Starting Kontact.  Delete all contacts and calendar "
		"entries manually.");
	}
	else {
		msg = _T(
		"Starting Kontact.  Delete all contacts and calendar "
		"entries manually (as well as memos and tasks if you are "
		"syncing them too)."
		);
	}
	int choice = wxMessageBox(msg, _T("Starting Kontact"),
			wxOK | wxCANCEL | wxICON_QUESTION, m_parent);
	if( choice != wxOK )
		return false;

	RunApp(parent);

	// Kontact forks, so we just have to trust that the user did it :-(
	return true;
}

} // namespace AppConfig

