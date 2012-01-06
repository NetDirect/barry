///
/// \file	MigrateDlg.h
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

#ifndef __BARRYDESKTOP_MIGRATEDLG_H__
#define __BARRYDESKTOP_MIGRATEDLG_H__

#include <wx/wx.h>
#include "deviceset.h"

class MigrateDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()	// sets to protected:

private:
	// external data sources
	const Barry::Probe::Results &m_results;
	int m_current_device_index;

	// in case we need to rescan the USB for a newly plugged device
	Barry::Probe::Results m_new_results;

	// dialog controls
	wxSizer *m_topsizer;
	wxComboBox *m_source_combo, *m_dest_combo, *m_write_mode_combo;
	wxButton *m_migrate_button;
	wxCheckBox *m_wipe_check;
	wxStaticText *m_status;
	wxGauge *m_progress;

protected:
	void CreateLayout();
	void AddDescriptionSizer(wxSizer *sizer);
	void AddMainSizer(wxSizer *sizer);
	void AddStatusSizer(wxSizer *sizer);

	void Main_AddSourceSizer(wxSizer *sizer);
	void Main_AddButtonSizer(wxSizer *sizer);
	void Main_AddDestSizer(wxSizer *sizer);

public:
	MigrateDlg(wxWindow *parent, const Barry::Probe::Results &results,
		int current_device_index = -1);

	// event handlers
	void OnMigrateNow(wxCommandEvent &event);
};

#endif

