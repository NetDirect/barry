///
/// \file	ConflictDlg.h
///		The dialog used during a sync, to display conflicting
///		changes, and let the user decide what to do.
///

/*
    Copyright (C) 2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_CONFLICTDLG_H__
#define __BARRYDESKTOP_CONFLICTDLG_H__

#include <wx/wx.h>
#include <vector>
#include "osbase.h"

class ConflictDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()

	// external data sources
	const std::vector<OpenSync::SyncChange> &m_changes;
	std::string m_supported_commands;

	// results
	std::string m_command_string;	// eg. "S 1"
	bool m_always;			// let user pick to automatically
					// repeat his last selection
					// FIXME should we allow user to
					// always pick the same plugin_name?
					// or always the first choice?
					// or something else more complicated?

	// dialog controls
//	wxSizer *m_topsizer, *m_appsizer;
//	wxComboBox *m_engine_combo, *m_app_combo;
//	wxTextCtrl *m_password_edit;
//	wxTextCtrl *m_status_edit;
//	wxCheckBox *m_debug_check;
	// Need: a method to display a variable number of changes
	//       and a variable number of buttons, to display the
	//       supported commands (Select, Duplicate, Ignore, Abort,
	//       Keep Newer, etc)

protected:
	void CreateLayout();
	void AddEngineSizer(wxSizer *sizer);
	void AddConfigSizer(wxSizer *sizer);
	void AddBarrySizer(wxSizer *sizer);
	void AddAppSizer(wxSizer *sizer);
	void UpdateAppSizer();		// called if engine changes, to update
					// the app combo, etc, with available
					// apps
	void LoadAppNames(wxArrayString &appnames);
	void AddButtonSizer(wxSizer *sizer);

public:
	ConflictDlg(wxWindow *parent, const std::string &supported_commands,
		const std::vector<OpenSync::SyncChange> &changes);
	~ConflictDlg();

	// data access
	const std::string& GetCommand() const { return m_command_string; }
	bool IsAlways() const { return m_always; }

	// event handlers
//	void OnConfigureApp(wxCommandEvent &event);
//	void OnEngineComboChange(wxCommandEvent &event);
//	void OnAppComboChange(wxCommandEvent &event);
};

#endif

