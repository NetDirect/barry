///
/// \file	CUI_Evolution.cc
///		ConfigUI derived class to configure the Evolution App
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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
}

Evolution::Evolution()
	: m_evolution(0)
	, m_parent(0)
{
}

bool Evolution::AutoDetect()
{
	if( m_evolution->AutoDetect() ) {
		// tell the user all went well
		wxMessageBox(_T("Evolution's configuration successfully auto-detected."), _T("Evolution Config"), wxOK | wxICON_INFORMATION, m_parent);
		return true;
	}
	else {
		return false;
	}
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

	// start Evolution, and wait for it to exit, showing a waiting
	// message to the user
	WaitDialog waitdlg(m_parent, _T("Waiting for Evolution to exit..."),
		_T("Evolution Running"));

	ExecCallback *callback = new ExecCallback(&waitdlg);

	const wxChar *shutdown_argv[] = {
		_T("evolution"),
		_T("--force-shutdown"),
		NULL
	};
	wxExecute((wxChar**)shutdown_argv, wxEXEC_SYNC);

	wxSleep(2);

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

	// if we get here, assume that the user followed our instructions
	// and attempt another autodetect
	if( AutoDetect() )
		return true;	// success!

	// and finally, failure
	wxMessageBox(_T("Failed to find Evolution's usual data locations.\n"
		"Please contact the developers.\n"
		"\n"
		"http://netdirect.ca/barry"),
		_T("Evolution Config"),
		wxOK | wxICON_ERROR,
		m_parent);
	return false;
}

bool Evolution::Configure(wxWindow *parent)
{
	m_parent = parent;

	// create our plugin config
	m_container.reset( m_evolution = new OpenSync::Config::Evolution );

	// if auto detect fails, fall back to starting Evolution
	// for the first time
	if( AutoDetect() || InitialRun() ) {
		return true;
	}
	else {
		m_container.reset();
		return false;
	}
}

ConfigUI::plugin_ptr Evolution::GetPlugin()
{
	m_evolution = 0;
	return m_container;
}

} // namespace AppConfig

