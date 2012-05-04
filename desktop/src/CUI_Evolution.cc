///
/// \file	CUI_Evolution.cc
///		ConfigUI derived class to configure the Evolution App
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

#include "CUI_Evolution.h"
#include "EvoSources.h"
#include "EvoCfgDlg.h"
#include "EvoDefaultDlg.h"
#include "os22.h"			// only for the dynamic_cast
#include "windowids.h"
#include <wx/wx.h>
#include <wx/process.h>

namespace AppConfig {

namespace {
	class ExecCallback : public wxProcess
	{
	public:
		wxDialog *m_waitdlg;
		int m_pid;
		int m_status;

		ExecCallback(wxDialog *dlg)
			: m_waitdlg(dlg)
			, m_pid(0)
			, m_status(0)
		{
		}

		virtual void OnTerminate(int pid, int status)
		{
			m_pid = pid;
			m_status = status;

			if( m_waitdlg ) {
				// close the dialog and let it delete us
				m_waitdlg->EndModal(wxID_OK);
			}
			else {
				// our parent doesn't exist, delete ourselves
				delete this;
			}
		}
	};

	class WaitDialog : public wxDialog
	{
		DECLARE_EVENT_TABLE()
	public:
		WaitDialog(wxWindow *parent, const wxString &msg,
			const wxString &title)
			: wxDialog(parent, wxID_ANY, title)
		{
			wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
			top->Add( new wxStaticText(this, wxID_ANY, msg),
				0, wxALL | wxALIGN_LEFT, 10);
			top->Add( new wxButton(this, wxID_CANCEL,
						_T("Stop waiting")),
				0, wxALL | wxALIGN_RIGHT, 5);
			SetSizer(top);
			top->SetSizeHints(this);
			top->Layout();

			SetEscapeId(wxID_CANCEL);
		}

		void OnButton(wxCommandEvent &event)
		{
			EndModal(wxID_CANCEL);
		}
	};

	BEGIN_EVENT_TABLE(WaitDialog, wxDialog)
		EVT_BUTTON	(wxID_CANCEL, WaitDialog::OnButton)
	END_EVENT_TABLE()
} // namespace

//////////////////////////////////////////////////////////////////////////////
// Static utility functions

long Evolution::ForceShutdown()
{
	ExecHelper shutdown(NULL);
	shutdown.Run(NULL, "Evolution shutdown", _T("evolution --force-shutdown"));
	shutdown.WaitForChild();
	return shutdown.GetChildExitCode();
}


//////////////////////////////////////////////////////////////////////////////
// EvolutionPtrBase class

EvolutionPtrBase::EvolutionPtrBase()
	: m_evolution(0)
{
}

void EvolutionPtrBase::AcquirePlugin(plugin_ptr old_plugin)
{
	// create our plugin config
	m_evolution = dynamic_cast<OpenSync::Config::Evolution*> (old_plugin.get());
	if( m_evolution ) {
		m_evolution = m_evolution->Clone();
	}
	else {
		m_evolution = new OpenSync::Config::Evolution;
	}
	m_container.reset( m_evolution );
}

void EvolutionPtrBase::Clear()
{
	m_container.reset();
	m_evolution = 0;
}

ConfigUI::plugin_ptr EvolutionPtrBase::GetPlugin()
{
	m_evolution = 0;
	return m_container;
}

//////////////////////////////////////////////////////////////////////////////
// Evolution config UI class

Evolution::Evolution()
	: m_parent(0)
{
}

bool Evolution::InitialRun()
{
	wxString msg = _T(
		"Unable to automatically detect Evolution's configuration.\n"
		"You need to run Evolution, and manually click each of the\n"
		"section buttons:\n"
		"\n"
		"      Mail, Contacts, Calendars, Memos, and Tasks\n"
		"\n"
		"Then quit Evolution to continue configuration.\n"
		"\n"
		"Would you like to start Evolution now?\n");

	int choice = wxMessageBox(msg, _T("Evolution Config"),
		wxYES_NO | wxICON_QUESTION, m_parent);

	if( choice == wxNO )
		return false;

	ForceShutdown();

	// start Evolution, and wait for it to exit, showing a waiting
	// message to the user
	WaitDialog waitdlg(m_parent, _T("Waiting for Evolution to exit..."),
		_T("Evolution Running"));

	ExecCallback *callback = new ExecCallback(&waitdlg);

	const wxChar *start_argv[] = {
		_T("evolution"),
		NULL
	};
	long ret = wxExecute((wxChar**)start_argv, wxEXEC_ASYNC, callback);
	if( ret == 0 ) {
		delete callback;
		wxMessageBox(_T("Failed to run evolution. Please make sure it is installed and in your PATH."), _T("Evolution Config"), wxOK | wxICON_ERROR, m_parent);
		return false;
	}

	if( waitdlg.ShowModal() == wxID_CANCEL ) {
		// user aborted, so ExecCallback will be left
		// waiting, and waitdlg will go out of scope, so
		// reset the callback's pointer
		callback->m_waitdlg = 0;
		return false;	// user aborted
	}
	else {
		// if we don't get wxID_CANCEL, then the callback
		// closed us, and we can delete it
		if( callback->m_status ) {
			// error status code
			wxMessageBox(_T("Failed to run evolution. Please make sure it is installed and in your PATH."), _T("Evolution Config"), wxOK | wxICON_ERROR, m_parent);
			delete callback;
			return false;
		}
		else {
			delete callback;
		}
	}

	// so far so good...
	return true;
}

std::string Evolution::AppName() const
{
	return OpenSync::Config::Evolution::AppName();
}

bool Evolution::Configure(wxWindow *parent, plugin_ptr old_plugin)
{
	m_parent = parent;

	AcquirePlugin(old_plugin);

	// auto detect first
	EvoSources srcs;

	// if no sources are found at all, and if Evolution is in the path
	//	do InitialRun and then auto detect again
	if( srcs.IsEmpty() ) {
		if( !InitialRun() ) {
			// impossible to do initial run, so fail here
			Clear();
			return false;
		}
		srcs.Detect();
	}

	// we now have an auto detect (EvoSources) to work with
	// if minimum three paths are available, and if no list has
	//	more than 1 item, then just default to those settings
	//	and notify the user... in the notification, allow
	//	the user to "Manual Cfg..." button
	bool manual = false;
	if( srcs.IsDefaultable() ) {
		EvoDefaultDlg dlg(m_parent);
		if( dlg.ShowModal() == Dialog_EvoDefault_ManualConfigButton ) {
			manual = true;
		}
	}

	// otherwise, if default settings are not possible, then
	//	load the path config dialog without notification
	if( !srcs.IsDefaultable() || manual ) {
		EvoCfgDlg cfgdlg(m_parent, *GetEvolutionPtr(), srcs);
		if( cfgdlg.ShowModal() == wxID_OK ) {
			cfgdlg.SetPaths(*GetEvolutionPtr());
		}
		else {
			Clear();
			return false;
		}
	}
	else {
		// it's defaultable!  use default paths
		GetEvolutionPtr()->SetAddressPath(srcs.GetAddressBook().size() ?
			srcs.GetAddressBook()[0].m_SourcePath : "");
		GetEvolutionPtr()->SetCalendarPath(srcs.GetEvents().size() ?
			srcs.GetEvents()[0].m_SourcePath : "");
		GetEvolutionPtr()->SetTasksPath(srcs.GetTasks().size() ?
			srcs.GetTasks()[0].m_SourcePath : "");
		GetEvolutionPtr()->SetMemosPath(srcs.GetMemos().size() ?
			srcs.GetMemos()[0].m_SourcePath : "");
	}

	// success!
	return true;
}

bool Evolution::RunApp(wxWindow *parent)
{
	return Run(parent, AppName(), _T("evolution"));
}

void Evolution::PreSyncAppInit()
{
	ForceShutdown();
}

bool Evolution::ZapData(wxWindow *parent,
			plugin_ptr plugin,
			OpenSync::API *engine)
{
	m_parent = parent;

	// extract OpenSync::Config::Evolution from plugin
	// this *can* throw an exception if the wrong plugin is
	// passed in, but we want this... such an exception would
	// represent a bug in the app, not a runtime error
//	OpenSync::Config::Evolution &evo =
//		dynamic_cast<OpenSync::Config::Evolution&>(*plugin);

	if( IsAppRunning() ) {
		wxMessageBox(_T("Evolution already running."),
			_T("No Biscuit"), wxOK | wxICON_INFORMATION,
			m_parent);
		return false;
	}

	// tell the user what to do
	wxString msg;
	if( dynamic_cast<OpenSync::OpenSync22*>(engine) ) {
		msg = _T(
		"Starting Evolution.  Delete all contacts and calendar "
		"entries manually.");
	}
	else {
		msg = _T(
		"Starting Evolution.  Delete all contacts and calendar "
		"entries manually (as well as memos and tasks if you are "
		"syncing them too)."
		);
	}
	int choice = wxMessageBox(msg, _T("Starting Evolution"),
			wxOK | wxCANCEL | wxICON_QUESTION, m_parent);
	if( choice != wxOK )
		return false;

	RunApp(parent);

	// wait for app to finish... this is kinda lame, but
	// we don't want other Zaps to happen before this is finished
	while( IsAppRunning() )
		wxMilliSleep(500);

	return true;
}


//////////////////////////////////////////////////////////////////////////////
// Evolution3 class

Evolution3::Evolution3()
	: m_evolution3(0)
{
}

void Evolution3::AcquirePlugin(plugin_ptr old_plugin)
{
	// create our plugin config
	m_evolution3 = dynamic_cast<OpenSync::Config::Evolution3*> (old_plugin.get());
	if( m_evolution3 ) {
		m_evolution3 = m_evolution3->Clone();
	}
	else {
		m_evolution3 = new OpenSync::Config::Evolution3;
	}
	m_container.reset( m_evolution3 );
}

void Evolution3::Clear()
{
	m_container.reset();
	m_evolution3 = 0;
}

ConfigUI::plugin_ptr Evolution3::GetPlugin()
{
	m_evolution3 = 0;
	return m_container;
}


} // namespace AppConfig

