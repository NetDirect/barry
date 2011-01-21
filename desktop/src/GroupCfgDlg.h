///
/// \file	GroupCfgDlg.h
///		The configuration dialog used when a user double clicks
///		on a device in the device list.  It lets the user choose
///		the app to sync with Barry, as well as the engine to use.
///

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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
	typedef OpenSync::Config::Group::plugin_ptr	plugin_ptr;
	typedef std::map<std::string, plugin_ptr>	appcfg_map;
	typedef std::map<OpenSync::API*, appcfg_map>	engapp_map;

	DECLARE_EVENT_TABLE()

	// external data sources
	const DeviceEntry &m_device;
	OpenSync::APISet &m_apiset;

	// misc flags
	int m_app_count;

	// results of the configuration
	std::string m_device_name;
	std::string m_group_name;
	DeviceEntry::group_ptr m_group;
	DeviceEntry::extras_ptr m_extras;

	// in-process config results... i.e. the plugin config
	// is stored here, and can be overwritten as the user
	// keeps fiddling with the controls... a map of engines
	// and apps is kept, so that if the user changes engine
	// or app, and then changes back, his settings are not lost...
	// this also means that pre-existing configs from outside
	// the dialog are kept if possible, to help with re-saving
	// of opensync configs
	engapp_map m_plugins;			// map of engines/apps/plugins
	OpenSync::API *m_engine;		// current engine
	OpenSync::Config::Barry m_barry_plugin;
	std::string m_favour_plugin_name;	// an extra

	// dialog controls
	wxSizer *m_topsizer, *m_appsizer;
	wxComboBox *m_engine_combo, *m_app_combo;
	wxTextCtrl *m_password_edit, *m_name_edit;
	wxCheckBox *m_debug_check;
	wxRadioBox *m_favour_radios;

protected:
	void CreateLayout();
	void AddEngineSizer(wxSizer *sizer);
	void AddConfigSizer(wxSizer *sizer);
	void AddBarrySizer(wxSizer *sizer);
	void AddAppSizer(wxSizer *sizer);
	void UpdateAppSizer(bool relayout = true); // called if engine changes,
				// to update the app combo, etc, with available
				// apps
	void LoadAppNames(wxArrayString &appnames);
	void AddFavourSizer(wxSizer *sizer);
	void AddButtonSizer(wxSizer *sizer);

	void SelectCurrentEngine();
	void LoadBarryConfig();
	void SelectApplication(const std::string appname);
	void SelectFavour();

	std::string GetCurrentAppName() const;	// returns name of currently
					// selected app, for the currently
					// selected engine... if none selected,
					// returns empty string (size() == 0)
	plugin_ptr GetCurrentPlugin();

public:
	GroupCfgDlg(wxWindow *parent, const DeviceEntry &device,
		OpenSync::APISet &apiset);

	// results
	std::string GetDeviceName() const { return m_device_name; }
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

