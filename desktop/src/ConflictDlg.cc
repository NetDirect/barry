///
/// \file	ConflictDlg.cc
///		The dialog used during a sync, to display conflicting
///		changes, and let the user decide what to do.
///

/*
    Copyright (C) 2010-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "ConflictDlg.h"
#include "windowids.h"
#include "util.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <wx/statline.h>
using namespace std;

//////////////////////////////////////////////////////////////////////////////
// ConflictDlg class

BEGIN_EVENT_TABLE(ConflictDlg, wxDialog)
	EVT_BUTTON	(Dialog_Conflict_ShowButton1,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_ShowButton2,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_ShowButton3,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_ShowButton4,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_ShowButton5,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_ShowButton6,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_ShowButton7,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_ShowButton8,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_ShowButton9,
				ConflictDlg::OnShowButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton1,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton2,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton3,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton4,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton5,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton6,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton7,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton8,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_SelectButton9,
				ConflictDlg::OnSelectButton)
	EVT_BUTTON	(Dialog_Conflict_DuplicateButton,
				ConflictDlg::OnDuplicateButton)
	EVT_BUTTON	(Dialog_Conflict_AbortButton,
				ConflictDlg::OnAbortButton)
	EVT_BUTTON	(Dialog_Conflict_IgnoreButton,
				ConflictDlg::OnIgnoreButton)
	EVT_BUTTON	(Dialog_Conflict_KeepNewerButton,
				ConflictDlg::OnKeepNewerButton)
	EVT_BUTTON	(Dialog_Conflict_KillSyncButton,
				ConflictDlg::OnKillSyncButton)
	EVT_CHECKBOX	(Dialog_Conflict_AlwaysCheckbox,
				ConflictDlg::OnAlwaysCheckbox)
END_EVENT_TABLE()

ConflictDlg::ConflictDlg(wxWindow *parent,
			const OpenSync::API &engine,
			const std::string &supported_commands,
			const std::vector<OpenSync::SyncChange> &changes,
			ConflictDlg::AlwaysMemoryBlock &always)
	: wxDialog(parent, Dialog_Conflict, _T("Sync Conflict"))
	, m_engine(engine)
	, m_changes(changes)
	, m_supported_commands(supported_commands)
	, m_always(always)
	, m_kill_sync(false)
	, m_topsizer(0)
{
	// the max text width is the maximum width that any line of text
	// in a conflict summary can use... it is calculated with
	// (screen_width - window_border_width*2) / change_count
	m_max_text_width = (wxSystemSettings::GetMetric(wxSYS_SCREEN_X) -
		wxSystemSettings::GetMetric(wxSYS_BORDER_X) * 2 -
		10 * 2 -
		5 * 2 * m_changes.size())
		/ m_changes.size();

	// first, parse all change data
	ParseChanges();

	// create a global set of key names that contain differing data
	// between changes
	CreateDifferingKeyNameSet();

	// setup the raw GUI
	CreateLayout();
}

ConflictDlg::~ConflictDlg()
{
}

void ConflictDlg::ParseChanges()
{
	m_maps.clear();

	// create an xmlmap to start with, so we only parse the
	// map file once
	ostringstream oss;
	oss << m_engine.GetEngineName() << "/xmlmap";
	tr1::shared_ptr<XmlNodeMap> basemap;
	try {
		basemap.reset(new XmlNodeMap(GetBaseFilename(oss.str())) );
	}
	catch( std::runtime_error &e ) {
		basemap.reset();
	}

	for( size_t i = 0; i < m_changes.size(); i++ ) {
		XmlPair xp;
		xp.dom.reset( new xmlpp::DomParser );
		xp.dom->parse_memory_raw(
			(const unsigned char*) m_changes[i].printable_data.data(),
			m_changes[i].printable_data.size());

		if( basemap.get() ) {
			xp.map.reset( new XmlNodeMap(*basemap) );
			xp.map->ImportNodes(xp.dom->get_document()->get_root_node());
			xp.map->PurgeEmpties();
			xp.map->SortBySummary();
		}

		m_maps.push_back(xp);
	}
}

void ConflictDlg::CreateDifferingKeyNameSet()
{
	// start fresh
	m_differing_keys.clear();

	if( !m_maps.size() || !m_maps[0].map.get() )
		return;	// nothing to do

	// find a list of all available key names
	key_set all_keys;	// set of all keys names from all xmlmaps
	for( size_t i = 0; i < m_maps.size(); i++ ) {
		XmlNodeMap::iterator mi = m_maps[i].map->begin(),
			me = m_maps[i].map->end();

		for( ; mi != me; ++mi ) {
			all_keys.insert(mi->KeyName());
		}

	}

	// cycle through all keys and find ones that have changes
	for( key_set::iterator i = all_keys.begin(); i!=all_keys.end(); ++i ) {

		// find the mapping from the first nodemap
		XmlNodeMapping *first = m_maps[0].map->Find(*i);
		if( !first ) {
			// if a key does not exist in this map, then
			// it does in another, and therefore is "differing"
			m_differing_keys.insert(*i);
			continue;
		}

		// cycle through all remaining nodemaps, find the mapping that
		// matches the keyname, and compare... if any do not
		// exist, or do not match, add to the differing keys set
		for( size_t j = 1; j < m_maps.size(); j++ ) {
			XmlNodeMapping *next = m_maps[j].map->Find(*i);

			// compare!
			if( !next || *first != *next ) {
				m_differing_keys.insert(*i);
				break;
			}
		}
	}
}

//
//   +-barry-sync---------+ +-evo2-sync----------+ +-google-sync------+
//   |                    | |                    | |                  |
//   |  Adam Brandee      | | Adam Brandee       | | Adam Brandee     |
//   |  IBM Canada        | |                    | | IBM Canada       | < red
//   |  1-519-555-1212    | | 1-519-555-1212     | | 1-519-555-1111   | < red
//   |  abramble@ibm.com  | | abramble@ibm.com   | | abramble@ibm.com |
//   |       ----         | |       ----         | |       ----       |
//   |  123 Big Street    | | 123 Big Street     | | 123 Long St.     | < red
//   |                    | |                    | | Canada           | < red
//   +--------------------+ +--------------------+ +------------------+
//
//     [See XML] [Select]     [See XML] [Select]     [See XML] [Select]
//
//
// At the bottom of the dialog, display the rest of the optional buttons,
// like Duplicate, Abort, etc.  Also include a checkbox for "always"...
// figure out the best way to handle "always" selections... always
// change #1?
//
// If converting an XML change's data to a hash map throws an exception
// somewhere in the xmlpp::DomParser, then that change will have to
// be handled in a raw manner.  I don't think any changes can be
// displayed in a table like above, but each should get a scrolling
// edit control with the raw data included.
//
// If possible, take the default table font and reduce it by 20% or
// something, because this table will likely hold a lot of data.
//
void ConflictDlg::CreateLayout()
{
	m_topsizer = new wxBoxSizer(wxVERTICAL);
	CreateSummaries(m_topsizer);
	CreateAlternateButtons(m_topsizer);

	SetSizer(m_topsizer);
	m_topsizer->SetSizeHints(this);
	m_topsizer->Layout();
}

void ConflictDlg::CreateSummaries(wxSizer *sizer)
{
	// this sizer is the horizontal box containing one
	// "map summary" each
	wxBoxSizer *box = new wxBoxSizer(wxHORIZONTAL);

	for( size_t i = 0; i < m_maps.size(); i++ ) {
		CreateSummary(box, i);
	}

	sizer->Add(box, 0, wxEXPAND | wxALL, 5);
}

void ConflictDlg::CreateSummary(wxSizer *sizer, size_t change_index)
{
	// this sizer contains the summary box in the top and the
	// buttons in the bottom
	wxBoxSizer *box = new wxBoxSizer(wxVERTICAL);

	CreateSummaryGroup(box, change_index);

	sizer->Add(box, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
}

void ConflictDlg::CreateSummaryGroup(wxSizer *sizer, size_t change_index)
{
	wxString name(m_changes[change_index].plugin_name.c_str(), wxConvUTF8);
	wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this, name);

	XmlNodeMap *xml = m_maps[change_index].map.get();
	if( xml ) {
		// add all priority 0 mappings
		XmlNodeMap::iterator nmi = xml->begin(), nme = xml->priority_end();
		for( ; nmi != nme; ++nmi ) {
			AddMapping(box, *nmi, IsDifferent(*nmi));
		}

		// add small separator (line?)
		box->Add( new wxStaticLine(this), 0, wxEXPAND | wxALL, 5);

		// add all differing mappings, in the map order, to preserve
		// the map file's user-friendly display order
		nmi = nme;	// start at priority_end() to skip priority 0
		nme = xml->end();
		for( ; nmi != nme; ++nmi ) {
			if( !IsDifferent(*nmi) )
				continue;

			AddMapping(box, *nmi, true);
		}
	}
	else {
		AddEmptyNotice(box);
	}

	box->Add(0, 10, 0);
	box->Add(0, 0, 1);
	CreateSummaryButtons(box, change_index);

	sizer->Add(box, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 0);
}

void ConflictDlg::CreateSummaryButtons(wxSizer *sizer, size_t change_index)
{
	wxBoxSizer *box = new wxBoxSizer(wxHORIZONTAL);

	box->Add(0, 0, 1);
	box->Add( new wxButton(this,
			Dialog_Conflict_ShowButton1 + change_index,
			_T("XML..."),
			wxDefaultPosition, wxDefaultSize,
			wxBU_EXACTFIT),
		0, wxTOP | wxLEFT | wxRIGHT, 3);
	box->Add( new wxButton(this,
			Dialog_Conflict_SelectButton1 + change_index,
			_T("Select"),
			wxDefaultPosition, wxDefaultSize,
			wxBU_EXACTFIT),
		0, wxTOP | wxLEFT | wxRIGHT, 3);

	sizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 0);
}

bool ConflictDlg::IsDifferent(const XmlNodeMapping &mapping) const
{
	return m_differing_keys.find(mapping.KeyName()) != m_differing_keys.end();
}

void ConflictDlg::AddEmptyNotice(wxSizer *sizer)
{
	wxFont font = GetFont();
	font.SetStyle( wxFONTSTYLE_ITALIC );

	wxStaticText *text = new wxStaticText(this, wxID_ANY,
		_T("No XML map found."),
		wxDefaultPosition, wxDefaultSize,
		wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
	text->SetFont(font);
	sizer->Add( text, 0, wxEXPAND, 0);
}

void ConflictDlg::AddMapping(wxSizer *sizer,
				XmlNodeMapping &mapping,
				bool differing)
{
// display differing text in red?  Not sure how to do that... using italic

	// use a big bold font for the high priority items
	wxFont priority_font = GetFont();
	priority_font.SetWeight( wxFONTWEIGHT_BOLD );

	// italic for changed items
	wxFont priority_changed = priority_font;
	priority_changed.SetStyle( wxFONTSTYLE_ITALIC );

	wxFont changed = GetFont();
	changed.SetStyle( wxFONTSTYLE_ITALIC );

	for( size_t i = 0; i < mapping.size(); i++ ) {
		wxString data(mapping[i].Summary().raw().c_str(), wxConvUTF8);

		wxStaticText *text = new wxStaticText(this, wxID_ANY, data,
				wxDefaultPosition, wxDefaultSize,
				wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
		if( mapping.Priority() == 0 ) {
			if( differing )
				text->SetFont(priority_changed);
			else
				text->SetFont(priority_font);
		}
		else {
			if( differing )
				text->SetFont(changed);
		}
		text->Wrap(m_max_text_width);

		sizer->Add( text, 0, wxEXPAND, 0);
	}
}

void ConflictDlg::CreateAlternateButtons(wxSizer *sizer)
{
	wxBoxSizer *box = new wxBoxSizer(wxHORIZONTAL);

	box->Add( new wxCheckBox(this, Dialog_Conflict_AlwaysCheckbox,
			_T("Always use this choice")),
		0, wxALIGN_CENTRE, 0);
	box->Add( -1, -1, 1 );

	if( m_supported_commands.find('D') != string::npos ) {
		box->Add( new wxButton(this, Dialog_Conflict_DuplicateButton,
					_T("Duplicate")),
			0, wxLEFT , 5);
	}

	if( m_supported_commands.find('I') != string::npos ) {
		box->Add( new wxButton(this, Dialog_Conflict_IgnoreButton,
					_T("Ignore")),
			0, wxLEFT, 5);
	}

	if( m_supported_commands.find('N') != string::npos ) {
		box->Add( new wxButton(this, Dialog_Conflict_KeepNewerButton,
					_T("Keep Newer")),
			0, wxLEFT, 5);
	}

	if( m_supported_commands.find('A') != string::npos ) {
		box->Add( new wxButton(this, Dialog_Conflict_AbortButton,
					_T("Abort")),
			0, wxLEFT, 5);
	}
	else {
		// no abort available, so add a Kill Sync button
		box->Add( new wxButton(this, Dialog_Conflict_KillSyncButton,
					_T("Kill Sync")),
			0, wxLEFT, 5);
	}

	sizer->Add(box, 0, wxEXPAND | wxALL, 10);
}


//////////////////////////////////////////////////////////////////////////////
// public members

void ConflictDlg::clear()
{
	m_command_string.clear();
	m_kill_sync = false;
}

int ConflictDlg::ShowModal()
{
	int ret = 0;

	// start fresh
	clear();

	// is there a favoured plugin name?
	if( m_always.m_favour_plugin_name.size() ) {
		// find the matching plugin name
		for( size_t i = 0; i < m_changes.size(); i++ ) {
			if( m_changes[i].plugin_name == m_always.m_favour_plugin_name ) {
				ostringstream oss;
				oss << "S " << m_changes[i].id;
				m_command_string = oss.str();
				return ret;
			}
		}
	}

	// is there an "always" answer we can use?
	if( m_always.m_always ) {
		if( m_always.m_last_command == "S" ) {
			// find the change that has a matching member_id
			// and plugin_name
			for( size_t i = 0; i < m_changes.size(); i++ ) {
				if( m_changes[i].member_id == m_always.m_member_id && m_changes[i].plugin_name == m_always.m_plugin_name ) {
					ostringstream oss;
					oss << "S " << m_changes[i].id;
					m_command_string = oss.str();
					return ret;
				}
			}
		}
		else {
			m_command_string = m_always.m_last_command;
			return ret; // done
		}
	}

	// ok, no "always" data that works, so ask the user
	do {
		ret = wxDialog::ShowModal();
	} while( m_command_string.size() == 0 && !m_kill_sync );

	return ret;
}

void ConflictDlg::OnShowButton(wxCommandEvent &event)
{
	int index = event.GetId() - Dialog_Conflict_ShowButton1;
	wxString xmldata(m_changes[index].printable_data.c_str(), wxConvUTF8);

	// let's try this the quick manual way...
	wxDialog dlg(this, wxID_ANY, _T("Raw Change Data"));
	wxBoxSizer *box = new wxBoxSizer(wxVERTICAL);
	box->Add( new wxTextCtrl(&dlg, wxID_ANY, xmldata,
			wxDefaultPosition, wxSize(400, 400),
			wxTE_MULTILINE | wxTE_READONLY),
		1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
	box->Add( dlg.CreateButtonSizer(wxOK), 0, wxEXPAND | wxALL, 10 );
	dlg.SetSizer(box);
	box->SetSizeHints(&dlg);
	box->Layout();
	dlg.ShowModal();
}

void ConflictDlg::OnSelectButton(wxCommandEvent &event)
{
	int index = event.GetId() - Dialog_Conflict_SelectButton1;

	// store command in m_always
	m_always.m_member_id = m_changes[index].member_id;
	m_always.m_plugin_name = m_changes[index].plugin_name;
	m_always.m_last_command = "S";

	// build actual command
	ostringstream oss;
	oss << "S " << m_changes[index].id;
	m_command_string = oss.str();

	EndModal(event.GetId());
}

void ConflictDlg::OnDuplicateButton(wxCommandEvent &event)
{
	m_command_string = m_always.m_last_command = "D";
	EndModal(event.GetId());
}

void ConflictDlg::OnAbortButton(wxCommandEvent &event)
{
	m_command_string = m_always.m_last_command = "A";
	EndModal(event.GetId());
}

void ConflictDlg::OnIgnoreButton(wxCommandEvent &event)
{
	m_command_string = m_always.m_last_command = "I";
	EndModal(event.GetId());
}

void ConflictDlg::OnKeepNewerButton(wxCommandEvent &event)
{
	m_command_string = m_always.m_last_command = "N";
	EndModal(event.GetId());
}

void ConflictDlg::OnKillSyncButton(wxCommandEvent &event)
{
	m_command_string.clear();
	m_always.m_last_command.clear();
	m_kill_sync = true;
	EndModal(event.GetId());
}

void ConflictDlg::OnAlwaysCheckbox(wxCommandEvent &event)
{
	m_always.m_always = event.IsChecked();
}

