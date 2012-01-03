///
/// \file	ConflictDlg.h
///		The dialog used during a sync, to display conflicting
///		changes, and let the user decide what to do.
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

#ifndef __BARRYDESKTOP_CONFLICTDLG_H__
#define __BARRYDESKTOP_CONFLICTDLG_H__

#include <wx/wx.h>
#include <vector>
#include <set>
#include "osbase.h"
#include "xmlmap.h"

class ConflictDlg : public wxDialog
{
public:
	typedef std::tr1::shared_ptr<XmlNodeMap>	map_ptr;
	typedef std::tr1::shared_ptr<xmlpp::DomParser>	dom_ptr;

	struct XmlPair
	{
		ConflictDlg::dom_ptr dom;	// never null
		ConflictDlg::map_ptr map;	// can contain null!
	};

	typedef std::vector<XmlPair>			mapped_list;
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
		std::string m_favour_plugin_name;

		void clear()
		{
			m_always = false;
			m_member_id = -1;
			m_plugin_name.clear();
			m_last_command.clear();
			m_favour_plugin_name.clear();
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
	const OpenSync::API &m_engine;
	const std::vector<OpenSync::SyncChange> &m_changes;
	std::string m_supported_commands;	// a char string: "SDAIN"
	AlwaysMemoryBlock &m_always;

	// mapped data
	mapped_list m_maps;
	key_set m_differing_keys;

	// results
	bool m_kill_sync;
	std::string m_command_string;	// eg. "S 1"

	// dialog sizing
	int m_max_text_width;

	// dialog controls
	wxSizer *m_topsizer;

protected:
	void CreateLayout();
	void CreateSummaries(wxSizer *sizer);
	void CreateSummary(wxSizer *sizer, size_t change_index);
	void CreateSummaryGroup(wxSizer *sizer, size_t change_index);
	void CreateSummaryButtons(wxSizer *sizer, size_t change_index);
	bool IsDifferent(const XmlNodeMapping &mapping) const;
	void AddEmptyNotice(wxSizer *sizer);
	void AddMapping(wxSizer *sizer, XmlNodeMapping &mapping,
		bool differing);
	void CreateAlternateButtons(wxSizer *sizer);

	void ParseChanges();
	void CreateDifferingKeyNameSet();

public:
	ConflictDlg(wxWindow *parent, const OpenSync::API &engine,
		const std::string &supported_commands,
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
	void OnShowButton(wxCommandEvent &event);
	void OnSelectButton(wxCommandEvent &event);
	void OnDuplicateButton(wxCommandEvent &event);
	void OnAbortButton(wxCommandEvent &event);
	void OnIgnoreButton(wxCommandEvent &event);
	void OnKeepNewerButton(wxCommandEvent &event);
	void OnKillSyncButton(wxCommandEvent &event);
	void OnAlwaysCheckbox(wxCommandEvent &event);
};

#endif

