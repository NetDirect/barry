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

	// Stored by the caller to remember the "always" selection
	// Pass it back into the dialog to have it automatically
	// select for you.
	struct AlwaysMemoryBlock
	{
		bool m_always;
		long m_member_id;
		std::string m_plugin_name;
		std::string m_last_command;

		void clear()
		{
			m_always = false;
			m_member_id = -1;
			m_plugin_name.clear();
			m_last_command.clear();
		}

		AlwaysMemoryBlock()
			: m_always(false)
			, m_member_id(-1)
		{
		}
	};

private:
	DECLARE_EVENT_TABLE()

	// external data sources
	const std::vector<OpenSync::SyncChange> &m_changes;
	std::string m_supported_commands;	// a char string: "SDAIN"
	AlwaysMemoryBlock &m_always;

	// parsed data
	parsed_list m_parsed;
	key_set m_key_set;

	// results
	bool m_kill_sync;
	std::string m_command_string;	// eg. "S 1"

	// dialog position calculation helpers
	int m_key_column_width;

	// dialog controls
	wxSizer *m_topsizer;
	wxListCtrl *m_data_list;

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
		const std::vector<OpenSync::SyncChange> &changes,
		AlwaysMemoryBlock &always);
	~ConflictDlg();

	// data access
	const std::string& GetCommand() const { return m_command_string; }
	bool IsKillSync() const { return m_kill_sync; }
	void Clear() { clear(); }
	void clear();

	// override, in order to force an answer
	int ShowModal();

	// event handlers
	void OnColumnButton(wxCommandEvent &event);
	void OnDuplicateButton(wxCommandEvent &event);
	void OnAbortButton(wxCommandEvent &event);
	void OnIgnoreButton(wxCommandEvent &event);
	void OnKeepNewerButton(wxCommandEvent &event);
	void OnKillSyncButton(wxCommandEvent &event);
	void OnAlwaysCheckbox(wxCommandEvent &event);
};

#endif

