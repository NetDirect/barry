///
/// \file	GroupCfgDlg.cc
///		The configuration dialog used when a user double clicks
///		on a device in the device list.  It lets the user choose
///		the app to sync with Barry, as well as the engine to use.
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "GroupCfgDlg.h"
#include "windowids.h"
#include <string>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// GroupCfgDlg class

GroupCfgDlg::GroupCfgDlg(wxWindow *parent,
			const DeviceEntry &device,
			OpenSync::API *device_engine,
			OpenSync::APISet &apiset)
	: wxDialog(parent, Dialog_GroupCfg, _T("Device Sync Configuration"))
	, m_device(device)
	, m_apiset(apiset)
	, m_engine(device_engine)
	, m_topsizer(0)
	, m_appsizer(0)
	, m_engine_combo(0)
	, m_app_combo(0)
	, m_password_edit(0)
	, m_debug_check(0)
{
	using namespace OpenSync;

	// make a copy of our group config if it is available
	if( m_device.IsConfigured() ) {
		m_group.reset( new Config::Group( *m_device.GetConfigGroup() ) );
		m_engine = device_engine; //m_device.GetEngine();
	}
	else {
		// start with an empty group
		string group_name = "barrydesktop_" + m_device.GetPin().str();
		m_group.reset( new Config::Group(group_name) );

		m_engine = 0;
	}

	// make sure our group always has a Barry config
	if( !m_group->HasBarryPlugins() ) {
		m_group->AddPlugin( new Config::Barry(m_device.GetPin()) );
	}

	// setup the GUI side
	CreateLayout();
	SelectCurrentEngine();
}

void GroupCfgDlg::CreateLayout()
{
	m_topsizer = new wxBoxSizer(wxVERTICAL);
	AddEngineSizer(m_topsizer);
	AddConfigSizer(m_topsizer);
	AddButtonSizer(m_topsizer);

	SetSizer(m_topsizer);
	m_topsizer->SetSizeHints(this);
	m_topsizer->Layout();
}

void GroupCfgDlg::AddEngineSizer(wxSizer *sizer)
{
	wxSizer *engine = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _T("OpenSync Engine")),
		wxHORIZONTAL
		);

	wxArrayString engines;
	if( m_apiset.os22() )
		engines.Add(wxString(m_apiset.os22()->GetVersion(),wxConvUTF8));
	if( m_apiset.os40() )
		engines.Add(wxString(m_apiset.os40()->GetVersion(),wxConvUTF8));

	engine->Add(
		m_engine_combo = new wxComboBox(this,
			Dialog_GroupCfg_EngineCombo, _T(""),
			wxDefaultPosition, wxSize(100, -1), engines,
			wxCB_READONLY),
		1, wxALL, 5);

	sizer->Add(engine, 0, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 10);
}

void GroupCfgDlg::AddConfigSizer(wxSizer *sizer)
{
	wxSizer *config = new wxBoxSizer(wxHORIZONTAL);
	AddBarrySizer(config);
	AddAppSizer(config);

	sizer->Add(config, 1, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 10);
}

void GroupCfgDlg::AddBarrySizer(wxSizer *sizer)
{
	wxSizer *barry = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _T("Barry Config")),
		wxVERTICAL
		);

	wxSizer *password = new wxBoxSizer(wxHORIZONTAL);
	password->Add(
		new wxStaticText(this, wxID_ANY, _T("Password:")),
		0, wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL, 2);
	password->Add(
		m_password_edit = new wxTextCtrl(this, wxID_ANY, _T(""),
			wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD),
		1, wxALIGN_LEFT, 0);
	barry->Add(password, 0, wxALL | wxEXPAND, 5);

	barry->Add(
		m_debug_check = new wxCheckBox(this, wxID_ANY,
			_T("Debug output during sync")),
		0, wxALIGN_LEFT, 5);

	sizer->Add(barry, 0, wxRIGHT | wxEXPAND, 5);
}

void GroupCfgDlg::AddAppSizer(wxSizer *sizer)
{
	m_appsizer = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _T("Application")),
		wxVERTICAL
		);

	UpdateAppSizer();

	sizer->Add(m_appsizer, 0, wxLEFT | wxEXPAND, 5);
}

void GroupCfgDlg::UpdateAppSizer()
{
	// start fresh
	m_appsizer->Clear(true);

	wxArrayString appnames;
	LoadAppNames(appnames);
	appnames.Sort();

	// FIXME - make size of combobox the size of the longest
	// string in apps?  using textextent calcs
	m_appsizer->Add(
		m_app_combo = new wxComboBox(this,
			Dialog_GroupCfg_AppCombo, _T(""),
			wxDefaultPosition, wxSize(200, -1), appnames,
			wxCB_READONLY),
		1, wxALL | wxALIGN_CENTRE, 5);
	m_appsizer->Add(
		new wxButton(this, Dialog_GroupCfg_AppConfigButton,
			_T("&Configure...")),
		0, wxALL | wxALIGN_CENTRE, 5);
}

void GroupCfgDlg::LoadAppNames(wxArrayString &appnames)
{
	// need to load app names based on engine plugin availability
	// NOTE - do not load the Barry plugin, since that's already assumed

	if( !m_engine ) {
		// no engine available
		appnames.Add(_T("No engine selected"));
		return;
	}

	using namespace OpenSync;
	string_list_type plugins;
	try {
		m_engine->GetPluginNames(plugins);
	}
	catch( std::exception &e ) {
		barrylog("Exception caught in LoadAppNames: " << e.what());
		return;
	}

	if( plugins.size() == 0 ) {
		appnames.Add(_T("No supported plugins available"));
		return;
	}

	// cycle through all available plugins, and add the ones
	// that we support
	string_list_type::const_iterator i = plugins.begin();
	for( ; i != plugins.end(); ++i ) {
		string appname;
		if( m_engine->GetConverter().IsPluginSupported(*i, &appname) ) {
			// found a supported plugin...

			// skip Barry
			if( appname == Config::Barry::AppName() )
				continue;

			appnames.Add( wxString(appname.c_str(), wxConvUTF8) );
		}
	}
}

void GroupCfgDlg::AddButtonSizer(wxSizer *sizer)
{
	wxSizer *button = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
	sizer->Add(button, 0, wxALL | wxEXPAND | wxALIGN_RIGHT, 10);
}

void GroupCfgDlg::SelectCurrentEngine()
{
	if( m_engine_combo ) {
		if( !m_engine ) {
			m_engine = m_apiset.os22() ?
				m_apiset.os22() : m_apiset.os40();
		}

		if( m_engine )
			m_engine_combo->SetValue(
				wxString(m_engine->GetVersion(), wxConvUTF8));

		UpdateAppSizer();
	}
}

