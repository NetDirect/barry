///
/// \file	SyncStatusDlg.h
///		The dialog used during a sync, to display status messages
///		and error messages, and handle sync conflicts via callback.
///

/*
    Copyright (C) 2010-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "configui.h"
#include "ConflictDlg.h"

class SyncStatusDlg;

class StatusConnection : public wxConnection, public OptOut::Element
{
	SyncStatusDlg &m_dlg;
	wxTextCtrl &m_status;

public:
	StatusConnection(SyncStatusDlg &dlg, wxTextCtrl &window);

	bool OnPoke(const wxString &topic, const wxString &item,
		wxChar *data, int size, wxIPCFormat format);

	// wxWidgets bug override - stop the 'delete this' behaviour,
	// since sometimes events seem to come through after
	// the delete... not sure why, and hard to debug.
	// This is with wxWidgets 2.8.7.
	//
	// With this override, the container in SyncStatusDlg
	// will handle all the deleting.
	virtual bool OnDisconnect() { return true; }
};

class ConflictConnection : public wxConnection, public OptOut::Element
{
	SyncStatusDlg &m_dlg;
	SillyBuffer m_buf;

	// conflict state machine
	bool m_asking_user;
	int m_current_sequenceID;
	int m_current_offset;
	int m_expected_total_changes;
	std::string m_supported_commands;
	std::vector<OpenSync::SyncChange> m_changes;

	// "always" memory
	ConflictDlg::AlwaysMemoryBlock m_always;

public:
	ConflictConnection(SyncStatusDlg &dlg);

	bool OnPoke(const wxString &topic, const wxString &item,
		wxChar *data, int size, wxIPCFormat format);
	wxChar* OnRequest(const wxString &topic, const wxString &item,
		int *size, wxIPCFormat format);

	// wxWidgets bug override - stop the 'delete this' behaviour,
	// since sometimes events seem to come through after
	// the delete... not sure why, and hard to debug.
	// This is with wxWidgets 2.8.7.
	//
	// With this override, the container in SyncStatusDlg
	// will handle all the deleting.
	virtual bool OnDisconnect() { return true; }
};

class SyncStatusDlg
	: public wxDialog
	, public wxServer
	, public TermCatcher
{
	DECLARE_EVENT_TABLE()	// sets to protected:

private:
	// external data sources
	DeviceSet::subset_type m_subset;
	DeviceSet::subset_type::iterator m_next_device, m_current_device;

	// for handling bsyncjail
	ExecHelper m_jailexec;
	std::string m_device_id;
	bool m_killingjail;

	// for handling run app
	ConfigUI::ptr m_ui;

	// connection holder, to make sure they get deleted if we
	// go out of scope
	OptOut::Vector<wxConnectionBase> m_connections;

	wxTimer m_timer;

	// dialog controls
	wxSizer *m_topsizer;
	wxStaticText *m_short_status;
	wxGauge *m_throbber;
	wxTextCtrl *m_status_edit;
	wxButton *m_runapp_button, *m_syncagain_button, *m_killclose_button;
	wxButton *m_details_button;

	// state
	bool m_repositioned;

protected:
	void CreateLayout();
	void AddStatusSizer(wxSizer *sizer);
	void AddButtonSizer(wxSizer *sizer);

	// set buttons to "running" state
	void SetRunning();
	// set buttons to "close" state
	void SetClose();

	void UpdateTitle();
	void UpdateLastSyncTime();

public:
	SyncStatusDlg(wxWindow *parent, const DeviceSet::subset_type &subset);
	~SyncStatusDlg();

	// operations
	void KillSync();
	void StartNextSync();

	void PrintStd(const std::string &msg, const wxColour &colour);
	void Print(const wxString &msg, const wxColour &colour);
	void ShortPrintStd(const std::string &msg);
	void ShortPrint(const wxString &msg);
	void Throb();
	void StartTimer();
	void StopTimer();

	DeviceEntry* GetCurrentDevice();

	// event handlers
	void OnSlowSync();	// called from StatusConnection
	void OnInitDialog(wxInitDialogEvent &event);
	void OnRunApp(wxCommandEvent &event);
	void OnSyncAgain(wxCommandEvent &event);
	void OnKillClose(wxCommandEvent &event);
	void OnShowDetails(wxCommandEvent &event);
	void OnExecTerminated(wxProcessEvent &event);
	void OnTimer(wxTimerEvent &event);

	// virtual overrides from wxServer
	wxConnectionBase* OnAcceptConnection(const wxString &topic);
};

#endif

