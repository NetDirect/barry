///
/// \file	configui.cc
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

#include "configui.h"
#include "osconfig.h"
#include "CUI_Evolution.h"

// Static factory function
ConfigUI::configui_ptr ConfigUI::CreateConfigUI(const std::string &appname)
{
	ConfigUI::configui_ptr ui;

	if( appname == OpenSync::Config::Evolution::AppName() ) {
		ui.reset( new AppConfig::Evolution );
	}

	return ui;
}

//////////////////////////////////////////////////////////////////////////////
// ConfigUI class

ConfigUI::ConfigUI()
	: m_app_callback(0)
	, m_app_pid(-1)
	, m_app_status(-1)
{
}

ConfigUI::~ConfigUI()
{
	if( m_app_callback ) {
		m_app_callback->Detach();
		m_app_callback = 0;
	}
}

bool ConfigUI::Run(wxWindow *parent, const wxChar *start_argv[])
{
	if( IsAppRunning() ) {
		RunError(parent, wxString(AppName().c_str(), wxConvUTF8) +
			_T(" is already running."));
		return false;
	}

	m_app_callback = new AppCallback(this);
	m_app_pid = wxExecute((wxChar**)start_argv, wxEXEC_ASYNC, m_app_callback);
	if( m_app_pid <= 0 ) {
		delete m_app_callback;
		m_app_callback = 0;
		m_app_pid = -1;

		RunError(parent, _T("Failed to run ") +
			wxString(AppName().c_str(), wxConvUTF8) +
			_T(". Please make sure it is installed and in your PATH."));
		return false;
	}

	wxMilliSleep(250);
	wxYield();
	if( !m_app_callback && m_app_status != 0 ) {
		RunError(parent, _T("Failed to run ") +
			wxString(AppName().c_str(), wxConvUTF8) +
			_T(". Please make sure it is installed and in your PATH."));
		return false;
	}

	return true;
}

void ConfigUI::RunError(wxWindow *parent, const wxString &msg)
{
	if( !parent )
		return;

	wxMessageBox(msg, _T("Application Run Error"),
		wxOK | wxICON_ERROR, parent);
}

bool ConfigUI::IsAppRunning()
{
	return m_app_callback && m_app_pid > 0;
}

void ConfigUI::KillApp(bool hardkill)
{
	if( IsAppRunning() ) {
		m_app_callback->Kill(m_app_pid, hardkill ? wxSIGKILL : wxSIGTERM);
		// let the callback handle the cleanup
	}
}

