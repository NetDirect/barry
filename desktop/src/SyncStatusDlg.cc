///
/// \file	SyncStatusDlg.cc
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

#include "SyncStatusDlg.h"
#include "ConflictDlg.h"
#include "windowids.h"
#include "configui.h"
#include "barrydesktop.h"
#include <wx/filename.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iterator>

using namespace std;

SillyBuffer sb;

//////////////////////////////////////////////////////////////////////////////
// StatusConnection class

StatusConnection::StatusConnection(SyncStatusDlg &dlg,
				wxTextCtrl &window)
	: m_dlg(dlg)
	, m_status(window)
{
}

bool StatusConnection::OnPoke(const wxString &topic,
				const wxString &item,
				wxChar *data,
				int size,
				wxIPCFormat format)
{
	if( topic != TOPIC_STATUS )
		return false;

	wxString msg(data, size);

	if( item == STATUS_ITEM_ERROR ) {
		m_dlg.Print(msg, *wxRED);
		m_dlg.ShortPrint(msg);
	}
	else if( item == STATUS_ITEM_ENTRY ) {
		m_dlg.Print(msg, *wxBLUE);
		m_dlg.ShortPrint("Syncing entries...");
	}
	else if( item == STATUS_ITEM_MAPPING ) {
		m_dlg.Print(msg, *wxBLUE);
	}
	else if( item == STATUS_ITEM_ENGINE ) {
		wxString key = ENGINE_STATUS_SLOW_SYNC;
		if( msg.substr(0, key.size()) == key ) {
			m_dlg.OnSlowSync();
		}
		else {
			m_dlg.Print(msg, *wxGREEN);
			m_dlg.ShortPrint(msg);
		}
	}
	else if( item == STATUS_ITEM_MEMBER ) {
		m_dlg.Print(msg, *wxCYAN);
	}
	else {
		// unknown item
		m_dlg.Print(msg, *wxBLACK);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////////
// ConflictConnection class

ConflictConnection::ConflictConnection(SyncStatusDlg &dlg)
	: m_dlg(dlg)
	, m_asking_user(false)
	, m_current_sequenceID(-1)
	, m_current_offset(-1)
	, m_expected_total_changes(0)
{
	// check if there's a favoured plugin name from the DeviceEntry config
	if( m_dlg.GetCurrentDevice() &&
	    m_dlg.GetCurrentDevice()->GetExtras() )
	{
		m_always.m_favour_plugin_name = m_dlg.GetCurrentDevice()->
					GetExtras()->m_favour_plugin_name;
	}
}

bool ConflictConnection::OnPoke(const wxString &topic,
				const wxString &item,
				wxChar *data,
				int size,
				wxIPCFormat format)
{
	barryverbose("Conflict::OnPoke: " << topic.utf8_str() << ", "
		<< item.utf8_str());

	if( topic != TOPIC_CONFLICT )
		return false;

	// if currently handling a user request, don't change
	// the state machine... the client shouldn't be poking
	// if he just Request'd anyway
	if( m_asking_user )
		return false;

	wxString msg(data, size);
	istringstream iss(string(msg.utf8_str()));

	if( item == CONFLICT_ITEM_START ) {
		// overwrite any existing sequence
		m_changes.clear();
		iss >> m_current_sequenceID
			>> m_current_offset
			>> m_expected_total_changes
			>> m_supported_commands;
		if( !iss || m_current_offset != 0 || m_expected_total_changes < 2) {
			// invalid start command, throw it away
			m_current_sequenceID = -1;
			m_current_offset = -1;
			m_expected_total_changes = 0;
			return false;
		}

		barryverbose("New conflict item: " << m_current_sequenceID
			<< ", " << m_current_offset << ", "
			<< "expected changes: " << m_expected_total_changes
			<< ", supported commands: " << m_supported_commands);
	}
	else if( item == CONFLICT_ITEM_CHANGE ) {
		int sequenceID = 0, offset = 0;
		OpenSync::SyncChange change;
		iss >> sequenceID >> offset >> change.id >> ws;
		getline(iss, change.plugin_name);
		getline(iss, change.uid);
		change.member_id = 0;

		if( !iss || sequenceID != m_current_sequenceID || offset != (m_current_offset + 1) ) {
			return false;
		}

		m_current_offset = offset;

		// grab remaining "printable data"
		copy((istreambuf_iterator<char>(iss)),
			(istreambuf_iterator<char>()),
			back_inserter(change.printable_data));

		m_changes.push_back(change);
		barryverbose("New conflict change: " << m_current_sequenceID
			<< ", " << m_current_offset << ", data: "
			<< change.printable_data);
	}

	return true;
}

wxChar* ConflictConnection::OnRequest(const wxString &topic,
					const wxString &item,
					int *size,
					wxIPCFormat format)
{
	// make sure we are in a valid sequence
	if( m_current_sequenceID == -1 || m_current_offset == -1 || m_expected_total_changes < 2) {
		barryverbose("Conflict: not in a valid sequence: "
			<< m_current_sequenceID << ", "
			<< m_current_offset << ", "
			<< m_expected_total_changes);
		return NULL;
	}

	// make sure we have a valid set of changes
	if( m_current_offset != m_expected_total_changes || (size_t)m_expected_total_changes != m_changes.size() ) {
		barryverbose("Conflict: not a valid set of changes: "
			<< m_current_offset << ", "
			<< m_expected_total_changes << ", "
			<< m_changes.size());
		return NULL;
	}

	m_asking_user = true;

	// ask the user what to do
	if( !m_dlg.GetCurrentDevice() ) {
		barryverbose("Conflict: current device is null");
		return NULL;
	}
	OpenSync::API *engine = m_dlg.GetCurrentDevice()->GetEngine();
	ConflictDlg dlg(&m_dlg, *engine, m_supported_commands,
		m_changes, m_always);
	m_dlg.StopTimer();
	dlg.ShowModal();
	m_dlg.StartTimer();

	// done
	m_asking_user = false;

	// did the user ask to kill the sync?
	if( dlg.IsKillSync() ) {
		// die!
		m_dlg.KillSync();
		return NULL;
	}

	// prepare response for client
	ostringstream oss;
	oss << m_current_sequenceID << " " << dlg.GetCommand();
	m_buf.buf(oss.str());
	// oddly, this is the size in bytes, not in wxChars
	*size = (m_buf.size() + 1) * sizeof(wxChar);
	return m_buf.buf();
}


//////////////////////////////////////////////////////////////////////////////
// SyncStatus class

BEGIN_EVENT_TABLE(SyncStatusDlg, wxDialog)
	EVT_INIT_DIALOG	(SyncStatusDlg::OnInitDialog)
	EVT_BUTTON	(Dialog_SyncStatus_RunAppButton,
				SyncStatusDlg::OnRunApp)
	EVT_BUTTON	(Dialog_SyncStatus_SyncAgainButton,
				SyncStatusDlg::OnSyncAgain)
	EVT_BUTTON	(Dialog_SyncStatus_KillCloseButton,
				SyncStatusDlg::OnKillClose)
	EVT_BUTTON	(Dialog_SyncStatus_ShowDetailsButton,
				SyncStatusDlg::OnShowDetails)
	EVT_END_PROCESS	(Dialog_SyncStatus_SyncTerminated,
				SyncStatusDlg::OnExecTerminated)
	EVT_TIMER	(Dialog_SyncStatus_Timer,
				SyncStatusDlg::OnTimer)
END_EVENT_TABLE()

SyncStatusDlg::SyncStatusDlg(wxWindow *parent,
				const DeviceSet::subset_type &subset)
	: wxDialog(parent, Dialog_SyncStatus, _T("Device Sync Progress"))
	, TermCatcher(this, Dialog_SyncStatus_SyncTerminated)
	, m_subset(subset)
	, m_next_device(m_subset.begin())
	, m_jailexec(this)
	, m_killingjail(false)
	, m_timer(this, Dialog_SyncStatus_Timer)
	, m_topsizer(0)
	, m_short_status(0)
	, m_throbber(0)
	, m_status_edit(0)
	, m_runapp_button(0)
	, m_syncagain_button(0)
	, m_killclose_button(0)
{
	wxBusyCursor wait;

	// setup the raw GUI
	CreateLayout();

	// create the IPC server
	wxServer::Create(SERVER_SERVICE_NAME);
}

SyncStatusDlg::~SyncStatusDlg()
{
	// make sure bsyncjail dies if we do
	m_jailexec.KillApp();
}

void SyncStatusDlg::CreateLayout()
{
	m_topsizer = new wxBoxSizer(wxVERTICAL);
	AddStatusSizer(m_topsizer);
	AddButtonSizer(m_topsizer);

	SetSizer(m_topsizer);
	m_topsizer->SetSizeHints(this);
	m_topsizer->Layout();
}

void SyncStatusDlg::AddStatusSizer(wxSizer *sizer)
{
	wxSizer *ss = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _T("Sync Progress")),
		wxVERTICAL
		);

	// add a set of short status and progress throbber
	wxSizer *shorts = new wxBoxSizer(wxHORIZONTAL);
	shorts->Add(
		m_short_status = new wxStaticText(this, wxID_ANY, _T(""),
			wxDefaultPosition, wxSize(350, -1),
			wxALIGN_LEFT),
		1, wxALIGN_CENTRE_VERTICAL | wxALL, 2);
	shorts->Add(
		m_throbber = new wxGauge(this, wxID_ANY, 0),
		0, wxALL | wxEXPAND, 2);
	ss->Add( shorts, 0, wxEXPAND, 0);

	ss->Add(
		m_status_edit = new wxTextCtrl(this, wxID_ANY, _T(""),
			wxDefaultPosition, wxSize(475, 450),
			wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH),
		0, wxALL | wxEXPAND, 2);
	// start with the details hidden
	m_status_edit->Hide();

	sizer->Add(ss, 1, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 5);
}

void SyncStatusDlg::AddButtonSizer(wxSizer *sizer)
{
	wxSizer *button = new wxBoxSizer(wxHORIZONTAL);

	button->Add(
		m_details_button = new wxButton(this,
			Dialog_SyncStatus_ShowDetailsButton,
			_T("Show Details")),
		0, wxALL, 3);

	button->Add( -1, -1, 1 );

	if( m_subset.size() == 1 ) {
		button->Add(
			m_runapp_button = new wxButton(this,
				Dialog_SyncStatus_RunAppButton,
				_T("Run App")),
			0, wxALL, 3);
	}
	button->Add(
		m_syncagain_button = new wxButton(this,
			Dialog_SyncStatus_SyncAgainButton,
			_T("Sync Again")),
		0, wxALL, 3);
	button->Add(
		m_killclose_button = new wxButton(this,
			Dialog_SyncStatus_KillCloseButton,
			_T("Kill Sync")),
		0, wxALL, 3);

	sizer->Add(button, 0, wxALL | wxEXPAND, 5);
}

void SyncStatusDlg::SetRunning()
{
	if( m_runapp_button )
		m_runapp_button->Enable(false);
	m_syncagain_button->Enable(false);
	m_killclose_button->SetLabel(_T("Kill Sync"));
	m_killclose_button->Enable(true);
	UpdateTitle();

	m_throbber->SetRange(10);
	m_throbber->SetValue(0);
	StartTimer();
}

void SyncStatusDlg::SetClose()
{
	if( m_runapp_button )
		m_runapp_button->Enable(true);
	m_syncagain_button->Enable(true);
	m_killclose_button->SetLabel(_T("Close"));
	m_killclose_button->Enable(true);
	UpdateTitle();

	m_throbber->SetRange(10);
	m_throbber->SetValue(10);
	StopTimer();
}

void SyncStatusDlg::Print(const std::string &msg, const wxColour &colour)
{
	Print(wxString(msg.c_str(), wxConvUTF8), colour);
}

void SyncStatusDlg::Print(const wxString &msg, const wxColour &colour)
{
	m_status_edit->SetDefaultStyle(wxTextAttr(colour));
	m_status_edit->AppendText(_T("\n") + msg);
}

void SyncStatusDlg::ShortPrint(const std::string &msg)
{
	ShortPrint(wxString(msg.c_str(), wxConvUTF8));
}

void SyncStatusDlg::ShortPrint(const wxString &msg)
{
	m_short_status->SetLabel(msg);
}

void SyncStatusDlg::Throb()
{
	m_throbber->Pulse();
}

void SyncStatusDlg::StartTimer()
{
	m_timer.Start(250);
}

void SyncStatusDlg::StopTimer()
{
	m_timer.Stop();
}

DeviceEntry* SyncStatusDlg::GetCurrentDevice()
{
	if( m_current_device == m_subset.end() )
		return 0;
	return &(*(*m_current_device));
}

void SyncStatusDlg::UpdateTitle()
{
	if( m_next_device == m_subset.end() ) {
		SetTitle(_T("Sync Progress Dialog"));
	}
	else {
		ostringstream oss;
		oss << "Syncing: " << (*m_next_device)->GetPin().Str()
		    << " with " << (*m_next_device)->GetAppNames();
		wxString label(oss.str().c_str(), wxConvUTF8);
		SetTitle(label);
	}
}

void SyncStatusDlg::UpdateLastSyncTime()
{
	if( m_current_device != m_subset.end() &&
	    (*m_current_device)->GetConfigGroup() &&
	    (*m_current_device)->GetExtras() )
	{
		(*m_current_device)->GetExtras()->m_last_sync_time = time(NULL);
		(*m_current_device)->GetExtras()->Save(
			wxGetApp().GetGlobalConfig(),
			(*m_current_device)->GetConfigGroup()->GetGroupName());
	}
}

void SyncStatusDlg::KillSync()
{
	m_jailexec.KillApp(m_killingjail);
	m_killingjail = true;

	// jump to the end of the sync roster, so we don't start the
	// next device
	m_current_device = m_next_device = m_subset.end();
}

void SyncStatusDlg::StartNextSync()
{
	m_killingjail = false;

	// anything to do?
	if( m_next_device == m_subset.end() ) {
		Print("No more devices to sync.", *wxBLACK);
		SetClose();
		return;
	}
	else {
		SetRunning();
	}

	// grab all required information we need to sync
	m_current_device = m_next_device;
	DeviceEntry &device = *(*m_next_device);
	const DeviceExtras *extras = device.GetExtras();
	m_device_id = device.GetPin().Str() + " (" + device.GetDeviceName() + ")";

	if( !device.IsConfigured() ) {
		Print(m_device_id + " is not configured, skipping.", *wxRED);
		ShortPrint("Skipping unconfigured: " + m_device_id);
		++m_next_device;
		m_current_device = m_subset.end();
		StartNextSync();
		return;
	}

	OpenSync::API *engine = device.GetEngine();
	OpenSync::Config::Group *group = device.GetConfigGroup();
	string group_name = group->GetGroupName();

	string statusmsg = "Starting sync for: " + m_device_id;
	Print(statusmsg, *wxBLACK);
	ShortPrint(statusmsg);

	// for each plugin / app, perform the pre-sync steps
	for( OpenSync::Config::Group::iterator i = group->begin();
		i != group->end();
		++i )
	{
		ConfigUI::ptr ui = ConfigUI::CreateConfigUI((*i)->GetAppName());
		if( ui.get() ) {
			ui->PreSyncAppInit();
		}
	}

	// initialize sync jail process
	if( m_jailexec.IsAppRunning() ) {
		string msg = "ERROR: App running in StartNextSync()";
		Print(msg, *wxRED);
		ShortPrint(msg);
		SetClose();
		return;
	}

	// assume that bsyncjail is in the same directory as barrydesktop
	wxFileName path(wxTheApp->argv[0]);
	wxString command = path.GetPath();
	// if command is empty, there is no path to the 'barrydesktop'
	// command, and therefore it was run via the PATH environment
	// variable... if the PATH was good enough for 'barrydesktop',
	// then it's good enough for 'bsyncjail' as well.  Skip the
	// separator so that we don't try to run '/bsyncjail'
	if( command.size() )
		command += path.GetPathSeparator();
	command += _T("bsyncjail ");
	command += wxString(engine->GetVersion(), wxConvUTF8);
	command += _T(" ");
	command += wxString(group_name.c_str(), wxConvUTF8);
	command += _T(" ");
	ostringstream sync_code;
	sync_code << dec << extras->m_sync_types;
	command += wxString(sync_code.str().c_str(), wxConvUTF8);

	if( !m_jailexec.Run(NULL, "bsyncjail", command) ) {
		Print("ERROR: unable to start bsyncjail: " + string(command.utf8_str()), *wxRED);
		ShortPrint("ERROR: unable to start bsyncjail");
		SetClose();
		return;
	}

	// sync is underway... advance to the next device
	++m_next_device;
}

void SyncStatusDlg::OnSlowSync()
{
	Print("Slow sync detected!  Killing sync automatically.", *wxRED);
	KillSync();

	Print("Slow syncs are known to be unreliable.", *wxBLACK);
	Print("Do a 1 Way Reset, and sync again.", *wxBLACK);
}

void SyncStatusDlg::OnInitDialog(wxInitDialogEvent &event)
{
	barryverbose("OnInitDialog");
	StartNextSync();
}

void SyncStatusDlg::OnRunApp(wxCommandEvent &event)
{
	if( m_subset.size() != 1 )
		return;

	if( m_ui.get() && m_ui->IsAppRunning() ) {
		wxMessageBox(_T("The application is already running."),
			_T("Run Application"), wxOK | wxICON_ERROR);
		return;
	}

	OpenSync::Config::Group *group = m_subset[0]->GetConfigGroup();
	if( !group )
		return;

	m_ui = ConfigUI::CreateConfigUI(group->GetAppNames());
	if( !m_ui.get() )
		return;

	m_ui->RunApp(this);
}

void SyncStatusDlg::OnSyncAgain(wxCommandEvent &event)
{
	m_next_device = m_subset.begin();
	StartNextSync();
}

void SyncStatusDlg::OnKillClose(wxCommandEvent &event)
{
	if( m_jailexec.IsAppRunning() ) {
		int choice;
		if( m_killingjail ) {
			choice = wxMessageBox(_T("Already killing sync.  Kill again?"),
				_T("Kill Sync"), wxYES_NO | wxICON_QUESTION);
		}
		else {
			choice = wxMessageBox(_T("This will kill the syncing child process, and will likely require a configuration reset.\n\nKill sync process anyway?"),
				_T("Kill Sync"), wxYES_NO | wxICON_QUESTION);
		}

		if( choice == wxYES ) {
			KillSync();

			// print a warning so the user know's what's going on
			Print("Killing sync... this may take a little while...", *wxRED);
			Print("Remember to re-plug your device.", *wxRED);

			// let the terminate call clean up the buttons
			return;
		}
	}
	else {
		EndModal(0);
	}
}

void SyncStatusDlg::OnShowDetails(wxCommandEvent &event)
{
	if( m_status_edit->IsShown() ) {
		m_status_edit->Hide();
		m_details_button->SetLabel(_T("Show Details"));
	}
	else {
		m_status_edit->Show();
		m_details_button->SetLabel(_T("Hide Details"));
	}
	m_topsizer->Fit(this);

	// try to position the window in a readable spot
	wxSize size = GetSize();
	wxPoint pos = GetScreenPosition();
	int screen_height = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
	if( (pos.y + size.GetHeight()) > screen_height &&
	    size.GetHeight() < screen_height )
	{
		int wiggle_room = screen_height - size.GetHeight();
		int y = wiggle_room / 2;
		Move(pos.x, y);
	}
}

wxConnectionBase* SyncStatusDlg::OnAcceptConnection(const wxString &topic)
{
	wxConnectionBase *con = 0;

	if( topic == TOPIC_STATUS && m_status_edit )
		con = new StatusConnection(*this, *m_status_edit);
	else if( topic == TOPIC_CONFLICT )
		con = new ConflictConnection(*this);

	if( con )
		m_connections.push_back( dynamic_cast<OptOut::Element*> (con) );

	return con;
}

void SyncStatusDlg::OnExecTerminated(wxProcessEvent &event)
{
	ostringstream oss;
	if( m_killingjail )
		oss << "Sync terminated: ";
	else
		oss << "Sync finished: ";
	oss << m_device_id;
	if( m_jailexec.GetAppStatus() )
		oss << " with error " << m_jailexec.GetAppStatus();
	Print(oss.str(), *wxBLACK);
	ShortPrint(oss.str());
	UpdateLastSyncTime();

	m_current_device = m_subset.end();

	StartNextSync();
}

void SyncStatusDlg::OnTimer(wxTimerEvent &event)
{
	Throb();
}

