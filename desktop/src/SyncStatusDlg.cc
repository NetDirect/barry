///
/// \file	SyncStatusDlg.cc
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

#include "SyncStatusDlg.h"
#include "ConflictDlg.h"
#include "windowids.h"
#include "configui.h"
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

	if( item == STATUS_ITEM_ERROR ) {
		m_status.SetDefaultStyle(wxTextAttr(*wxRED));
		m_status.AppendText(_T("\n") + wxString(data, size));
	}
	else if( item == STATUS_ITEM_ENTRY ) {
		m_status.SetDefaultStyle(wxTextAttr(*wxBLUE));
		m_status.AppendText(_T("\n") + wxString(data, size));
	}
	else if( item == STATUS_ITEM_MAPPING ) {
		m_status.SetDefaultStyle(wxTextAttr(*wxBLUE));
		m_status.AppendText(_T("\n") + wxString(data, size));
	}
	else if( item == STATUS_ITEM_ENGINE ) {
		wxString msg(data, size);
		wxString key = ENGINE_STATUS_SLOW_SYNC;
		if( msg.substr(0, key.size()) == key ) {
			m_dlg.OnSlowSync();
		}
		else {
			m_status.SetDefaultStyle(wxTextAttr(*wxGREEN));
			m_status.AppendText(_T("\n") + wxString(data, size));
		}
	}
	else if( item == STATUS_ITEM_MEMBER ) {
		m_status.SetDefaultStyle(wxTextAttr(*wxCYAN));
		m_status.AppendText(_T("\n") + wxString(data, size));
	}
	else {
		// unknown item
		m_status.SetDefaultStyle(wxTextAttr(*wxBLACK));
		m_status.AppendText(_T("\n") + wxString(data, size));
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////////
// ConflictConnection class

ConflictConnection::ConflictConnection(wxWindow *parent)
	: m_parent(parent)
	, m_asking_user(false)
	, m_current_sequenceID(-1)
	, m_current_offset(-1)
	, m_expected_total_changes(0)
{
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
	ConflictDlg dlg(m_parent, m_supported_commands, m_changes);
	dlg.ShowModal();

	ostringstream oss;
	oss << m_current_sequenceID << " " << dlg.GetCommand();

	// FIXME
	//if( dlg.IsAlways() ) {
		// save this command for repeated automatic action
	//}

	// done
	m_asking_user = false;

	// prepare response for client
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
	EVT_END_PROCESS	(Dialog_SyncStatus_SyncTerminated,
				SyncStatusDlg::OnExecTerminated)
END_EVENT_TABLE()

SyncStatusDlg::SyncStatusDlg(wxWindow *parent,
				const DeviceSet::subset_type &subset)
	: wxDialog(parent, Dialog_SyncStatus, _T("Device Sync Progress"),
		wxDefaultPosition, wxSize(500,500))
	, TermCatcher(this, Dialog_SyncStatus_SyncTerminated)
	, m_subset(subset)
	, m_next_device(m_subset.begin())
	, m_jailexec(this)
	, m_killingjail(false)
	, m_topsizer(0)
	, m_status_edit(0)
	, m_runapp_button(0)
	, m_syncagain_button(0)
	, m_killclose_button(0)
{
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
//	m_topsizer->SetSizeHints(this);
	m_topsizer->Layout();
}

void SyncStatusDlg::AddStatusSizer(wxSizer *sizer)
{
	wxSizer *ss = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _T("Sync Progress")),
		wxHORIZONTAL
		);

	ss->Add(
		m_status_edit = new wxTextCtrl(this, wxID_ANY, _T(""),
			wxDefaultPosition, wxDefaultSize,
			wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH),
		1, wxALL | wxEXPAND, 2);

	sizer->Add(ss, 1, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 5);
}

void SyncStatusDlg::AddButtonSizer(wxSizer *sizer)
{
	wxSizer *button = new wxBoxSizer(wxHORIZONTAL);

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

	sizer->Add(button, 0, wxALL | wxALIGN_RIGHT, 5);
}

void SyncStatusDlg::SetRunning()
{
	if( m_runapp_button )
		m_runapp_button->Enable(false);
	m_syncagain_button->Enable(false);
	m_killclose_button->SetLabel(_T("Kill Sync"));
	m_killclose_button->Enable(true);
}

void SyncStatusDlg::SetClose()
{
	if( m_runapp_button )
		m_runapp_button->Enable(true);
	m_syncagain_button->Enable(true);
	m_killclose_button->SetLabel(_T("Close"));
	m_killclose_button->Enable(true);
}

void SyncStatusDlg::PrintBlack(const std::string &msg)
{
	m_status_edit->SetDefaultStyle(wxTextAttr(*wxBLACK));
	m_status_edit->AppendText(_T("\n") + wxString(msg.c_str(), wxConvUTF8));
}

void SyncStatusDlg::PrintRed(const std::string &msg)
{
	m_status_edit->SetDefaultStyle(wxTextAttr(*wxRED));
	m_status_edit->AppendText(_T("\n") + wxString(msg.c_str(), wxConvUTF8));
}

void SyncStatusDlg::KillSync()
{
	m_jailexec.KillApp(m_killingjail);
	m_killingjail = true;

	// jump to the end of the sync roster, so we don't start the
	// next device
	m_next_device = m_subset.end();
}

void SyncStatusDlg::StartNextSync()
{
	m_killingjail = false;

	// anything to do?
	if( m_next_device == m_subset.end() ) {
		PrintBlack("No more devices to sync.");
		SetClose();
		return;
	}
	else {
		SetRunning();
	}

	// grab all required information we need to sync
	DeviceEntry &device = *(*m_next_device);
	m_device_id = device.GetPin().str() + " (" + device.GetDeviceName() + ")";

	if( !device.IsConfigured() ) {
		PrintRed(m_device_id + " is not configured, skipping.");
		++m_next_device;
		StartNextSync();
		return;
	}

	OpenSync::API *engine = device.GetEngine();
	OpenSync::Config::Group *group = device.GetConfigGroup();
	string group_name = group->GetGroupName();

	PrintBlack("Starting sync for: " + m_device_id);

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
		PrintRed("ERROR: App running in StartNextSync()");
		SetClose();
		return;
	}

	// assume that bsyncjail is in the same directory as barrydesktop
	wxFileName path(wxTheApp->argv[0]);
	wxString command = path.GetPath();
	command += path.GetPathSeparator();
	command += _T("bsyncjail ");
	command += wxString(engine->GetVersion(), wxConvUTF8);
	command += _T(" ");
	command += wxString(group_name.c_str(), wxConvUTF8);

	if( !m_jailexec.Run(NULL, "bsyncjail", command) ) {
		PrintRed("ERROR: unable to start bsyncjail: " + string(command.utf8_str()));
		SetClose();
		return;
	}

	// sync is underway... advance to the next device
	++m_next_device;
}

void SyncStatusDlg::OnSlowSync()
{
	PrintRed("Slow sync detected!  Killing sync automatically.");
	KillSync();

	PrintBlack("Slow syncs are known to be unreliable.");
	PrintBlack("Do a 1 Way Reset, and sync again.");
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
			PrintRed("Killing sync... this may take a little while...");
			PrintRed("Remember to re-plug your device.");

			// let the terminate call clean up the buttons
			return;
		}
	}
	else {
		EndModal(0);
	}
}

wxConnectionBase* SyncStatusDlg::OnAcceptConnection(const wxString &topic)
{
	wxConnectionBase *con = 0;

	if( topic == TOPIC_STATUS && m_status_edit )
		con = new StatusConnection(*this, *m_status_edit);
	else if( topic == TOPIC_CONFLICT )
		con = new ConflictConnection(this);

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
	PrintBlack(oss.str());

	StartNextSync();
}

