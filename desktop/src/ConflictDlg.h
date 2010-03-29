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
#include <set>
#include "osbase.h"
#include "xmlcompactor.h"	// unfortunately this pulls in Glib and libxml++

class ConflictDlg : public wxDialog
{
public:
	typedef std::tr1::shared_ptr<XmlCompactor>	xml_ptr;
	typedef std::vector<xml_ptr>			parsed_list;
	typedef std::set<Glib::ustring>			key_set;

private:
	DECLARE_EVENT_TABLE()

	// external data sources
	const std::vector<OpenSync::SyncChange> &m_changes;
	std::string m_supported_commands;	// a char string: "SDAIN"

	// parsed data
	parsed_list m_parsed;
	key_set m_key_set;

	// results
	std::string m_command_string;	// eg. "S 1"
	bool m_always;			// let user pick to automatically
					// repeat his last selection
					// FIXME should we allow user to
					// always pick the same plugin_name?
					// or always the first choice?
					// or something else more complicated?

	// dialog controls
	wxSizer *m_topsizer;
	wxListCtrl *m_data_list;
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
	void CreateTable(wxSizer *sizer);
	void CreateSelectorButtons(wxSizer *sizer);
	void CreateAlternateButtons(wxSizer *sizer);

	void ParseChanges();
	void CreateKeyNameSet();
	int GetWidestNameExtent(wxWindow *window);
	int GetWidestDataExtent(wxWindow *window, int change_index);
	Glib::ustring GetParsedData(int index, const Glib::ustring &key);
	bool IsChanged(const Glib::ustring &key);
	bool IsNew(const Glib::ustring &key);
	bool IsEqual(const Glib::ustring &key);
	void AddData(long item, const Glib::ustring &key);
	void FillDataList();
//	void LoadAppNames(wxArrayString &appnames);
//	void AddButtonSizer(wxSizer *sizer);

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

