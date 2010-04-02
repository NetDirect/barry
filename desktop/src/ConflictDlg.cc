///
/// \file	ConflictDlg.cc
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

#include "ConflictDlg.h"
#include "windowids.h"
#include <iostream>
#include <sstream>
using namespace std;

//////////////////////////////////////////////////////////////////////////////
// ConflictDlg class

BEGIN_EVENT_TABLE(ConflictDlg, wxDialog)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton1,
				ConflictDlg::OnColumnButton)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton2,
				ConflictDlg::OnColumnButton)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton3,
				ConflictDlg::OnColumnButton)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton4,
				ConflictDlg::OnColumnButton)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton5,
				ConflictDlg::OnColumnButton)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton6,
				ConflictDlg::OnColumnButton)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton7,
				ConflictDlg::OnColumnButton)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton8,
				ConflictDlg::OnColumnButton)
	EVT_BUTTON	(Dialog_Conflict_ColumnButton9,
				ConflictDlg::OnColumnButton)
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
			const std::string &supported_commands,
			const std::vector<OpenSync::SyncChange> &changes,
			ConflictDlg::AlwaysMemoryBlock &always)
	: wxDialog(parent, Dialog_Conflict, _T("Sync Conflict"))
	, m_changes(changes)
	, m_supported_commands(supported_commands)
	, m_always(always)
	, m_kill_sync(false)
	, m_key_column_width(0)
	, m_topsizer(0)
	, m_data_list(0)
{
	// first, parse all change data
	ParseChanges();

	// create a global set of key names from all parsed changes
	CreateKeyNameSet();

	// setup the raw GUI
	CreateLayout();

	// fill the conflict list
	FillDataList();
}

ConflictDlg::~ConflictDlg()
{
}

void ConflictDlg::ParseChanges()
{
	m_parsed.clear();

	for( size_t i = 0; i < m_changes.size(); i++ ) {
		xml_ptr parser( new XmlCompactor );
		parser->parse_memory_raw(
			(const unsigned char*) m_changes[i].printable_data.data(),
			m_changes[i].printable_data.size());
		parser->Map(parser->FindCommonPrefix());
		m_parsed.push_back(parser);
	}
}

void ConflictDlg::CreateKeyNameSet()
{
	m_key_set.clear();

	for( size_t i = 0; i < m_parsed.size(); i++ ) {
		XmlCompactor::iterator xi = m_parsed[i]->begin();
		for( ; xi != m_parsed[i]->end(); ++xi ) {
			m_key_set.insert(xi->first);
		}
	}
}

// cycle through the entire m_key_set, saving the largest
// text extent found
int ConflictDlg::GetWidestNameExtent(wxWindow *window)
{
	int largest = 0;
	key_set::iterator i = m_key_set.begin();
	for( ; i != m_key_set.end(); ++i ) {
		int width, y;
		window->GetTextExtent(wxString(i->raw().c_str(), wxConvUTF8), &width, &y);
		if( width > largest )
			largest = width;
	}

	return largest;
}

// cycle through every ->second data item in the XmlCompactor
// to find the largest text extent
int ConflictDlg::GetWidestDataExtent(wxWindow *window, int change_index)
{
	int largest = 0;
	XmlCompactor *xc = m_parsed[change_index].get();
	XmlCompactor::iterator i = xc->begin();

	for( ; i != xc->end(); ++i ) {
		int width, y;
		window->GetTextExtent(
			wxString(i->second.raw().c_str(), wxConvUTF8),
			&width, &y);

		if( width > largest )
			largest = width;
	}

	return largest;
}

//
// +----------------+----------------+---------------+---------------+
// |                |  change 1      |   change 2    |   change 3    |
// +----------------+----------------+---------------+---------------+
// | Address/City   | Kitchener      |               | Kitchener     |
// | Address/Country| Canada         | Canada        | Canada        |
// .....
//
// Display a table like above, listing all map keys from all XML-parsed
// change data, and if possible, highlight the rows with differences.
// Or at least display the font in that row with a non-black colour.
//
// Probably able to display this in a listctrl, but this makes it harder
// to connect the buttons with the change columns.  Maybe you can do it
// with some fancy math....
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
	CreateTable(m_topsizer);
	CreateAlternateButtons(m_topsizer);

	SetSizer(m_topsizer);
	m_topsizer->SetSizeHints(this);
	m_topsizer->Layout();
}

void ConflictDlg::CreateTable(wxSizer *sizer)
{
	wxStaticBoxSizer *box = new wxStaticBoxSizer(wxVERTICAL, this,
		_T("Conflicting Changes"));

	m_data_list = new wxListCtrl(this, Dialog_Conflict_DataList,
				wxDefaultPosition, wxDefaultSize,
				wxLC_REPORT /*| wxLC_VRULES*/);

	// use a slightly smaller font for all this data
	wxFont font = GetFont();
	// don't let the font get smaller than 6 point, unless the user's
	// default font is smaller than 6
	int base_size = std::min(font.GetPointSize(), 6);
	font.SetPointSize( std::max(font.GetPointSize() - 2, base_size) );
	m_data_list->SetFont(font);

	// get the text extent width of the widest name in the change list
	int widest = GetWidestNameExtent(m_data_list);
	m_key_column_width = widest;

	// insert 1 column for field names, and 1 each for each change
	m_data_list->InsertColumn(0, _T(""), wxLIST_FORMAT_LEFT, widest);
	int total_width = widest;
	for( size_t i = 1; i <= m_changes.size(); i++ ) {
		wxString title(m_changes[i-1].plugin_name.c_str(), wxConvUTF8);
		unsigned int width = GetWidestDataExtent(m_data_list, i-1);
		width = std::min(width, 700 / m_changes.size());

		// limit the width of the data to a total of 700 pixels
		// for all the data columns, which is pretty wide...
		m_data_list->InsertColumn(i, title, wxLIST_FORMAT_LEFT, width);
		total_width += width;
	}

	// make sure this doesn't go smaller than what we ask for
	m_data_list->SetMinSize( wxSize(total_width, 300) );

	// add to sizers
	box->Add( m_data_list, 1, wxEXPAND | wxALL, 4 );
	CreateSelectorButtons(box);

	sizer->Add(box, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
}

void ConflictDlg::CreateSelectorButtons(wxSizer *sizer)
{
	if( m_supported_commands.find('S') == std::string::npos )
		return;

	wxBoxSizer *box = new wxBoxSizer(wxHORIZONTAL);

	box->Add(m_key_column_width, -1, 0);
	for( size_t i = 0; i < m_changes.size(); i++ ) {
		wxString name(m_changes[i].plugin_name.c_str(), wxConvUTF8);
		name.Prepend(_T("Use: "));
		box->Add( new wxButton(this,
				Dialog_Conflict_ColumnButton1 + i,
				name),
			1, wxTOP | wxBOTTOM, 5);
	}

	sizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
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

	sizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
}

Glib::ustring ConflictDlg::GetParsedData(int index, const Glib::ustring &key)
{
	Glib::ustring value;
	XmlCompactor *xc = m_parsed[index].get();
	XmlCompactor::iterator i = xc->find(key);
	if( i != xc->end() )
		value = i->second;
	return value;
}

// returns true if all columns contain data, yet are different
bool ConflictDlg::IsChanged(const Glib::ustring &key)
{
	if( m_parsed.size() == 0 )
		return false;

	Glib::ustring data = GetParsedData(0, key);
	if( data.size() == 0 )
		return false;	// empty fields are "new"

	bool status = false;

	for( size_t index = 1; index < m_parsed.size(); index++ ) {
		Glib::ustring next = GetParsedData(index, key);
		if( next.size() == 0 )
			return false; // empty fields are still "new" :-)
		if( data != next )
			status = true;
	}
	return status;
}

// returns true if there is one different empty column
bool ConflictDlg::IsNew(const Glib::ustring &key)
{
	if( m_parsed.size() == 0 )
		return false;

	bool equal = true, empty = false;

	Glib::ustring data = GetParsedData(0, key);
	if( data.size() == 0 )
		empty = true;

	for( size_t index = 1; index < m_parsed.size(); index++ ) {
		Glib::ustring next = GetParsedData(index, key);
		if( next.size() == 0 )
			empty = true;
		if( data != next )
			equal = false;
	}
	return !equal && empty;
}

// returns true if all columns are equal, whether empty or not
bool ConflictDlg::IsEqual(const Glib::ustring &key)
{
	if( m_parsed.size() == 0 )
		return true;

	Glib::ustring data = GetParsedData(0, key);
	for( size_t index = 1; index < m_parsed.size(); index++ ) {
		Glib::ustring next = GetParsedData(index, key);
		if( data != next )
			return false;
	}
	return true;
}

void ConflictDlg::AddData(long item, const Glib::ustring &key)
{
	// loop through each set of parsed changes, and find
	// the key's data, if available, and add it to the
	// matching column
	for( size_t column = 1; column <= m_parsed.size(); column++ ) {
		Glib::ustring data = GetParsedData(column-1, key);
		wxString text(data.raw().c_str(), wxConvUTF8);
		m_data_list->SetItem(item, column, text);
	}
}

void ConflictDlg::FillDataList()
{
	// start fresh
	m_data_list->DeleteAllItems();

	// add an entry for each changed item in the m_key_set
	key_set::iterator i = m_key_set.begin();
	int list_index = 0;
	for( ; i != m_key_set.end(); ++i, list_index++ ) {

		// fill with changed items first
		if( !IsChanged(*i) )
			continue;

		wxString label(i->raw().c_str(), wxConvUTF8);
		long item = m_data_list->InsertItem(list_index, label);
		AddData(item, *i);
		m_data_list->SetItemTextColour(item, *wxRED);
	}

	// do it again, for the changed "new" items
	i = m_key_set.begin();
	for( ; i != m_key_set.end(); ++i, list_index++ ) {

		if( !IsNew(*i) )
			continue;

		wxString label(i->raw().c_str(), wxConvUTF8);
		long item = m_data_list->InsertItem(list_index, label);
		AddData(item, *i);
		m_data_list->SetItemTextColour(item, *wxBLUE);
	}

	// do it again, for the equal items
	i = m_key_set.begin();
	for( ; i != m_key_set.end(); ++i, list_index++ ) {

		// fill with changed items first
		if( !IsEqual(*i) )
			continue;

		wxString label(i->raw().c_str(), wxConvUTF8);
		long item = m_data_list->InsertItem(list_index, label);
		AddData(item, *i);
	}
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

void ConflictDlg::OnColumnButton(wxCommandEvent &event)
{
	int index = event.GetId() - Dialog_Conflict_ColumnButton1;

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

