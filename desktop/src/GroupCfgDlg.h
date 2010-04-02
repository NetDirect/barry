///
/// \file	GroupCfgDlg.h
///		The configuration dialog used when a user double clicks
///		on a device in the device list.  It lets the user choose
///		the app to sync with Barry, as well as the engine to use.
///

/*
    Copyright (C) 2009-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_GROUPCFGDLG_H__
#define __BARRYDESKTOP_GROUPCFGDLG_H__

#include <wx/wx.h>
#include "deviceset.h"

class GroupCfgDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()

	// external data sources
	const DeviceEntry &m_device;
	OpenSync::APISet &m_apiset;

	// results of the configuration
	std::string m_group_name;
	DeviceEntry::group_ptr m_group;
	DeviceEntry::extras_ptr m_extras;

	// in-process config results... i.e. the plugin config
	// is stored here, and can be overwritten as the user
	// keeps fiddling with the controls... at the end, if
	// valid, this config is added to the group for a
	// final configuration result
	OpenSync::API *m_engine;		// current engine
	OpenSync::Config::Barry m_barry_plugin;
	OpenSync::Config::Group::plugin_ptr m_app_plugin;
	std::string m_favour_plugin_name;	// an extra

	// dialog controls
	wxSizer *m_topsizer, *m_appsizer;
	wxComboBox *m_engine_combo, *m_app_combo;
	wxTextCtrl *m_password_edit;
	wxCheckBox *m_debug_check;
	wxRadioBox *m_favour_radios;

protected:
	void CreateLayout();
	void AddEngineSizer(wxSizer *sizer);
	void AddConfigSizer(wxSizer *sizer);
	void AddBarrySizer(wxSizer *sizer);
	void AddAppSizer(wxSizer *sizer);
	void UpdateAppSizer();		// called if engine changes, to update
					// the app combo, etc, with available
					// apps
	void LoadAppNames(wxArrayString &appnames);
	void AddFavourSizer(wxSizer *sizer);
	void AddButtonSizer(wxSizer *sizer);

	void SelectCurrentEngine();
	void LoadBarryConfig();
	void SelectApplication();
	void SelectFavour();
	bool IsAppConfigured();		// returns true if it is safe to
					// exit the dialog successfully, and
					// there's no more that needs to be
					// done before an opensync config save

public:
	GroupCfgDlg(wxWindow *parent, const DeviceEntry &device,
		OpenSync::APISet &apiset);

	// results
	DeviceEntry::group_ptr GetGroup() { return m_group; }
	OpenSync::API* GetEngine() const { return m_engine; }
	DeviceEntry::extras_ptr GetExtras() { return m_extras; }

	// event handlers
	void OnConfigureApp(wxCommandEvent &event);
	void OnEngineComboChange(wxCommandEvent &event);
	void OnAppComboChange(wxCommandEvent &event);

	// overrides
	virtual bool TransferDataFromWindow();
	int ShowModal();
};

#endif

