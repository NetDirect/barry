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

#include <iostream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include "SyncStatusDlg.h"
#include "ConflictDlg.h"
#include "windowids.h"

using namespace std;

SillyBuffer sb;

//////////////////////////////////////////////////////////////////////////////
// StatusConnection class

StatusConnection::StatusConnection(wxTextCtrl &window)
	: m_status(window)
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
	if( topic != TOPIC_STATUS )
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
	}

	return true;
}

wxChar* ConflictConnection::OnRequest(const wxString &topic,
					const wxString &item,
					int *size,
					wxIPCFormat format)
{
	// make sure we are in a valid sequence
	if( m_current_sequenceID == -1 || m_current_offset == -1 || m_expected_total_changes < 2)
		return NULL;

	// make sure we have a valid set of changes
	if( m_current_offset != m_expected_total_changes || (size_t)m_expected_total_changes != m_changes.size() )
		return NULL;

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
//	EVT_BUTTON	(Dialog_GroupCfg_AppConfigButton,
//				GroupCfgDlg::OnConfigureApp)
//	EVT_TEXT	(Dialog_GroupCfg_EngineCombo,
//				GroupCfgDlg::OnEngineComboChange)
//	EVT_TEXT	(Dialog_GroupCfg_AppCombo,
//				GroupCfgDlg::OnAppComboChange)
END_EVENT_TABLE()

SyncStatusDlg::SyncStatusDlg(wxWindow *parent,
				const std::string &group_name,
				OpenSync::API &engine)
	: wxDialog(parent, Dialog_SyncStatus, _T("Device Sync Progress"))
	, m_group_name(group_name)
	, m_engine(engine)
	, m_status_edit(0)
{
	// setup the raw GUI
	CreateLayout();

	// setup status stream
//	blah;

	// initialize sync thread
//	blah;

	// create the IPC server
	wxServer::Create(SERVER_SERVICE_NAME);
}

SyncStatusDlg::~SyncStatusDlg()
{
}

void SyncStatusDlg::CreateLayout()
{
	m_status_edit = new wxTextCtrl(this, wxID_ANY, _T(""),
		wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
}

wxConnectionBase* SyncStatusDlg::OnAcceptConnection(const wxString &topic)
{
	if( topic == TOPIC_STATUS && m_status_edit )
		return new StatusConnection(*m_status_edit);
	else if( topic == TOPIC_CONFLICT )
		return new ConflictConnection(this);
	else
		return 0;
}
