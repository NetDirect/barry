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

//////////////////////////////////////////////////////////////////////////////
// ConflictDlg class

BEGIN_EVENT_TABLE(ConflictDlg, wxDialog)
//	EVT_BUTTON	(Dialog_GroupCfg_AppConfigButton,
//				GroupCfgDlg::OnConfigureApp)
//	EVT_TEXT	(Dialog_GroupCfg_EngineCombo,
//				GroupCfgDlg::OnEngineComboChange)
//	EVT_TEXT	(Dialog_GroupCfg_AppCombo,
//				GroupCfgDlg::OnAppComboChange)
END_EVENT_TABLE()

ConflictDlg::ConflictDlg(wxWindow *parent,
			const std::string &supported_commands,
			const std::vector<OpenSync::SyncChange> &changes)
	: wxDialog(parent, Dialog_Conflict, _T("Sync Conflict"))
	, m_changes(changes)
	, m_supported_commands(supported_commands)
	, m_always(false)
{
}

ConflictDlg::~ConflictDlg()
{
}

