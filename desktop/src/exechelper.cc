///
/// \file	exechelper.cc
///		Helper class to wrap wxProcess and wxExecute operations
///

/*
    Copyright (C) 2010, Chris Frey <cdfrey@foursquare.net>

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

#include "exechelper.h"

TermCatcher::~TermCatcher()
{
	if( m_eh )
		m_eh->m_catcher = 0;
}

ExecHelper::ExecHelper(TermCatcher *catcher)
	: m_catcher(catcher)
	, m_app_callback(0)
	, m_app_pid(-1)
	, m_app_status(-1)
{
	// link ourselves to the catcher... the catcher will unlink if necessary
	if( m_catcher )
		m_catcher->m_eh = this;
}

ExecHelper::~ExecHelper()
{
	if( m_app_callback ) {
		m_app_callback->Detach();
		m_app_callback = 0;
	}
	if( m_catcher )
		m_catcher->m_eh = 0;
}

void ExecHelper::RunError(wxWindow *parent, const wxString &msg)
{
	if( !parent )
		return;

	wxMessageBox(msg, _T("Application Run Error"),
		wxOK | wxICON_ERROR, parent);
}

bool ExecHelper::Run(wxWindow *parent,
			const std::string &appname,
			const wxString &command)
{
	if( IsAppRunning() ) {
		RunError(parent, wxString(appname.c_str(), wxConvUTF8) +
			_T(" is already running."));
		return false;
	}

	m_app_callback = new AppCallback(this);
	m_app_pid = wxExecute(command, wxEXEC_ASYNC, m_app_callback);
	if( m_app_pid <= 0 ) {
		delete m_app_callback;
		m_app_callback = 0;
		m_app_pid = -1;

		RunError(parent, _T("Failed to run ") +
			wxString(appname.c_str(), wxConvUTF8) +
			_T(". Please make sure it is installed and in your PATH."));
		return false;
	}

	return true;
}

bool ExecHelper::IsAppRunning()
{
	return m_app_callback && m_app_pid > 0;
}

void ExecHelper::KillApp(bool hardkill)
{
	if( IsAppRunning() ) {
		m_app_callback->Kill(m_app_pid, hardkill ? wxSIGKILL : wxSIGTERM);
		// let the callback handle the cleanup
	}
}

