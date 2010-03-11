///
/// \file	SyncStatusDlg.h
///		The dialog used during a sync, to display status messages
///		and error messages, and handle sync conflicts via callback.
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

#ifndef __BARRYDESKTOP_SYNCSTATUSDLG_H__
#define __BARRYDESKTOP_SYNCSTATUSDLG_H__

#include <wx/wx.h>
#include "ipc.h"
#include "osbase.h"
#include "deviceset.h"
#include "exechelper.h"
#include "optout.h"

class StatusConnection : public wxConnection, public OptOut::Element
{
	wxTextCtrl &m_status;

public:
	StatusConnection(wxTextCtrl &window);

	bool OnPoke(const wxString &topic, const wxString &item,
		wxChar *data, int size, wxIPCFormat format);
};

class ConflictConnection : public wxConnection, public OptOut::Element
{
	wxWindow *m_parent;
	SillyBuffer m_buf;

	// conflict state machine
	bool m_asking_user;
	int m_current_sequenceID;
	int m_current_offset;
	int m_expected_total_changes;
	std::string m_supported_commands;
	std::vector<OpenSync::SyncChange> m_changes;

public:
	ConflictConnection(wxWindow *parent);

	bool OnPoke(const wxString &topic, const wxString &item,
		wxChar *data, int size, wxIPCFormat format);
	wxChar* OnRequest(const wxString &topic, const wxString &item,
		int *size, wxIPCFormat format);
};

class SyncStatusDlg
	: public wxDialog
	, public wxServer
	, public TermCatcher
{
	DECLARE_EVENT_TABLE()

	// external data sources
	DeviceSet::subset_type m_subset;
	DeviceSet::subset_type::iterator m_next_device;

	// for handling bsyncjail
	ExecHelper m_exec;
	std::string m_device_id;

	// connection holder, to make sure they get deleted if we
	// go out of scope
	OptOut::Vector<wxConnectionBase> m_connections;

	// dialog controls
//	wxSizer *m_topsizer, *m_appsizer;
//	wxComboBox *m_engine_combo, *m_app_combo;
//	wxTextCtrl *m_password_edit;
	wxTextCtrl *m_status_edit;
//	wxCheckBox *m_debug_check;
	// Need: a pretty status edit box, to show all status messages,
	//       and a "sync action" button that says "Kill Sync"
	//       when running, and "Close" if not.
	//       and a button "Restart Sync" button that is greyed out
	//       when running, and a "Run App" button that is greyed out
	//       when running... remember to warn against killing a
	//       sync

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

	// set buttons to "close" configuration
	void SetClose();

	void PrintBlack(const std::string &msg);
	void PrintRed(const std::string &msg);

public:
	SyncStatusDlg(wxWindow *parent, const DeviceSet::subset_type &subset);
	~SyncStatusDlg();

	// operations
	void StartNextSync();

	// event handlers
	void OnInitDialog(wxInitDialogEvent &event);
//	void OnConfigureApp(wxCommandEvent &event);
//	void OnEngineComboChange(wxCommandEvent &event);
//	void OnAppComboChange(wxCommandEvent &event);

	// virtual overrides from wxServer
	wxConnectionBase* OnAcceptConnection(const wxString &topic);

	// virtual overrides from TermCatcher
	virtual void ExecTerminated();
};

#endif

