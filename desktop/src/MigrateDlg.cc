///
/// \file	MigrateDlg.cc
///		Dialog for the "Migrate Device" main menu mode button...
///		going with a dialog instead of a mode class this time.
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "MigrateDlg.h"
#include "windowids.h"
#include "configui.h"
#include "barrydesktop.h"
#include <string>
#include <wx/statline.h>

using namespace std;
using namespace OpenSync;

BEGIN_EVENT_TABLE(MigrateDlg, wxDialog)
	EVT_BUTTON	(Dialog_Migrate_MigrateNowButton,
				MigrateDlg::OnMigrateNow)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////////
// MigrateDlg class

MigrateDlg::MigrateDlg(wxWindow *parent,
			const Barry::Probe::Results &results,
			int current_device_index)
	: wxDialog(parent, Dialog_GroupCfg, _T("Migrate Device"))
	, m_results(results)
	, m_current_device_index(current_device_index)
{
	// setup the raw GUI
	CreateLayout();
}

void MigrateDlg::CreateLayout()
{
	m_topsizer = new wxBoxSizer(wxVERTICAL);

//	AddDescriptionSizer(m_topsizer);
	AddMainSizer(m_topsizer);
	AddStatusSizer(m_topsizer);

	SetSizer(m_topsizer);
	m_topsizer->SetSizeHints(this);
	m_topsizer->Layout();
}

void MigrateDlg::AddDescriptionSizer(wxSizer *sizer)
{
	sizer->Add(
		new wxStaticText(this, wxID_ANY,
			_T("Migrate device data from source device to target device.")),
		0, wxALIGN_CENTRE | wxALIGN_CENTRE_VERTICAL |
			wxTOP | wxLEFT | wxRIGHT, 10);
}

void MigrateDlg::AddMainSizer(wxSizer *sizer)
{
	// add 3 main sections together into one sizer
	wxSizer *main = new wxBoxSizer(wxHORIZONTAL);
	Main_AddSourceSizer(main);
	Main_AddButtonSizer(main);
	Main_AddDestSizer(main);

	// add main sizer to top level sizer
	sizer->Add(main, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
}

void MigrateDlg::AddStatusSizer(wxSizer *sizer)
{
	sizer->Add( new wxStaticLine(this),
		0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);

	sizer->Add(
		m_status = new wxStaticText(this, wxID_ANY, _T("Ready...")),
		0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	// Reduce font size for status text
	wxFont font = m_status->GetFont();
	font.SetPointSize(font.GetPointSize() - 1);
	m_status->SetFont( font );

	sizer->Add(
		m_progress = new wxGauge(this, wxID_ANY, 100),
		0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
}

void MigrateDlg::Main_AddSourceSizer(wxSizer *sizer)
{
	wxArrayString devices;
	for( Barry::Probe::Results::const_iterator i = m_results.begin();
				i != m_results.end(); ++i )
	{
		devices.Add(wxString(i->GetDisplayName().c_str(), wxConvUTF8));
	}

	wxSizer *source = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _T("Source device")),
		wxVERTICAL
		);
	source->Add(
		m_source_combo = new wxChoice(this, wxID_ANY,
			wxDefaultPosition, wxSize(225, -1), devices),
		0, wxALL, 5);

	if( m_current_device_index >= 0 )
		m_source_combo->SetSelection(m_current_device_index);

	sizer->Add(source, 0, wxEXPAND, 0);
}

void MigrateDlg::Main_AddButtonSizer(wxSizer *sizer)
{
	wxSizer *buttons = new wxBoxSizer(wxVERTICAL);
	buttons->Add( m_migrate_button = new wxButton(this,
		Dialog_Migrate_MigrateNowButton, _T("Migrate Now")),
		0, wxALIGN_CENTRE, 0);
	m_migrate_button->SetDefault();
	buttons->AddSpacer(10);
	buttons->Add( new wxButton(this, wxID_CANCEL, _T("Cancel")),
		0, wxALIGN_CENTRE, 0);

	sizer->Add(buttons, 1, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 10);
}

void MigrateDlg::Main_AddDestSizer(wxSizer *sizer)
{
	wxArrayString devices;
	devices.Add(_T("Prompt to plug in later..."));
	for( Barry::Probe::Results::const_iterator i = m_results.begin();
				i != m_results.end(); ++i )
	{
		devices.Add(wxString(i->GetDisplayName().c_str(), wxConvUTF8));
	}

	wxSizer *dest = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _T("Destination device")),
		wxVERTICAL
		);
	dest->Add(
		m_dest_combo = new wxChoice(this, wxID_ANY,
			wxDefaultPosition, wxSize(225, -1), devices),
		0, wxALL, 5);
	m_dest_combo->SetSelection(0);


	wxArrayString write_modes;
	write_modes.Add(_T("Erase all, then restore"));
	write_modes.Add(_T("Add new, and overwrite existing"));
	write_modes.Add(_T("Add only, don't overwrite existing"));
	write_modes.Add(_T("Add every record as a new entry (may cause duplicates)"));

	dest->Add( new wxStaticText(this, wxID_ANY, _T("Write Mode:")),
		0, wxTOP | wxLEFT | wxRIGHT, 5);
	dest->Add( m_write_mode_combo = new wxChoice(this, wxID_ANY,
			wxDefaultPosition, wxSize(225, -1), write_modes),
		0, wxALL, 5);
	m_write_mode_combo->SetSelection(0);

//	dest->Add( m_wipe_check = wxCheckBox(maybe a checkbox for "wipe device before restore"));

	sizer->Add(dest, 0, wxEXPAND, 0);
}



void MigrateDlg::OnMigrateNow(wxCommandEvent &event)
{
}

