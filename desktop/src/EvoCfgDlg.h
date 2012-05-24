///
/// \file	EvoCfgDlg.h
///		The configuration dialog used to configure Evolution sources
///

/*
    Copyright (C) 2011-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_EVOCFGDLG_H__
#define __BARRYDESKTOP_EVOCFGDLG_H__

#include <wx/wx.h>
#include "EvoSources.h"
#include "ostypes.h"

// forward declarations
namespace OpenSync {
	namespace Config {
		class Evolution;
	}
}

class EvoCfgDlg : public wxDialog
{
	// configuration settings
	std::string m_address_path;
	std::string m_calendar_path;
	std::string m_tasks_path;
	std::string m_memos_path;
	bool m_empty_config;		// true if all paths are empty, and
					// therefore the first time this
					// is running, and defaults my be
					// attempted

	// external data
	const EvoSources &m_sources;

	// dialog controls
	wxSizer *m_topsizer;
	wxComboBox *m_address_combo;
	wxComboBox *m_calendar_combo;
	wxComboBox *m_tasks_combo;
	wxComboBox *m_memos_combo;

protected:
	void CreateLayout();
	void AddCombo(wxComboBox **combo, int  id,
		const std::string &current_path,
		const EvoSources::List &list);
	wxString CheckPath(const wxString &name, const std::string &path) const;
	/// returns error message string on error, otherwise empty string
	wxString ValidatePaths() const;

public:
	EvoCfgDlg(wxWindow *parent, const OpenSync::Config::Evolution &ec,
		const EvoSources &es);

	// results
	void SetPaths(OpenSync::Config::Evolution &ec) const;

	// event handlers

	// overrides
	virtual bool TransferDataFromWindow();
	int ShowModal();
};

#endif

