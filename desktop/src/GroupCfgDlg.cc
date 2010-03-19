///
/// \file	GroupCfgDlg.cc
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

#include "GroupCfgDlg.h"
#include "windowids.h"
#include "configui.h"
#include <string>

using namespace std;
using namespace OpenSync;

BEGIN_EVENT_TABLE(GroupCfgDlg, wxDialog)
	EVT_BUTTON	(Dialog_GroupCfg_AppConfigButton,
				GroupCfgDlg::OnConfigureApp)
	EVT_TEXT	(Dialog_GroupCfg_EngineCombo,
				GroupCfgDlg::OnEngineComboChange)
	EVT_TEXT	(Dialog_GroupCfg_AppCombo,
				GroupCfgDlg::OnAppComboChange)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////////
// GroupCfgDlg class

GroupCfgDlg::GroupCfgDlg(wxWindow *parent,
			const DeviceEntry &device,
			OpenSync::APISet &apiset)
	: wxDialog(parent, Dialog_GroupCfg, _T("Device Sync Configuration"))
	, m_device(device)
	, m_apiset(apiset)
	, m_engine(0)
	, m_barry_plugin(m_device.GetPin())
	, m_topsizer(0)
	, m_appsizer(0)
	, m_engine_combo(0)
	, m_app_combo(0)
	, m_password_edit(0)
	, m_debug_check(0)
{
	// setup the raw GUI
	CreateLayout();

	// set window title to device PIN and name
	string label = "Configure Device - ";
	label += m_device.GetPin().str();
	if( m_device.GetDeviceName().size() )
		label += " (" + m_device.GetDeviceName() + ")";
	SetTitle(wxString(label.c_str(), wxConvUTF8));

	// initialize current engine pointer
	if( m_device.GetEngine() ) {
		m_engine = const_cast<OpenSync::API*> (m_device.GetEngine());
	}

	if( m_device.GetConfigGroup() ) {
		const Config::Group *group = m_device.GetConfigGroup();
		// use existing group name, if available
		m_group_name = group->GetGroupName();

		// copy Barry plugin config, if available
		if( group->HasBarryPlugins() )
			m_barry_plugin = group->GetBarryPlugin();

		// copy non-Barry plugin config, if available
		const Config::Plugin *plugin = group->GetNonBarryPlugin();
		if( plugin )
			m_app_plugin.reset( plugin->Clone() );
	}
	else {
		m_group_name = "barrydesktop_" + m_device.GetPin().str();
	}

	SelectCurrentEngine();
	LoadBarryConfig();
	SelectApplication();
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

void GroupCfgDlg::LoadBarryConfig()
{
	Config::Barry &bp = m_barry_plugin;

	m_password_edit->SetValue( wxString(bp.GetPassword().c_str(), wxConvUTF8) );
	m_debug_check->SetValue( bp.IsDebugMode() );
}

void GroupCfgDlg::SelectApplication()
{
	if( m_app_plugin.get() ) {
		m_app_combo->SetValue(
			wxString(m_app_plugin->GetAppName().c_str(), wxConvUTF8));
	}
}

void GroupCfgDlg::OnConfigureApp(wxCommandEvent &event)
{
	wxString app = m_app_combo->GetValue();
	if( app.size() == 0 ) {
		wxMessageBox(_T("Please select an application."),
			_T("Application Config"), wxOK | wxICON_ERROR, this);
		return;
	}

	ConfigUI::ptr ui = ConfigUI::CreateConfigUI(std::string(app.utf8_str()));

	if( !ui.get() ) {
		wxMessageBox(_T("No configuration interface available for this Application."),
			_T("Application Config"),
			wxOK | wxICON_ERROR, this);
		return;
	}

	if( ui->Configure(this) ) {
		ConfigUI::plugin_ptr plugin = ui->GetPlugin();
		if( plugin.get() ) {
			m_app_plugin = plugin;
		}
	}
}

void GroupCfgDlg::OnEngineComboChange(wxCommandEvent &event)
{
	// update engine pointer
	wxString newEngine = m_engine_combo->GetValue();
	if( m_apiset.os22() && newEngine == wxString(m_apiset.os22()->GetVersion(), wxConvUTF8) ) {
		m_engine = m_apiset.os22();
	}
	else if( m_apiset.os40() && newEngine == wxString(m_apiset.os40()->GetVersion(), wxConvUTF8) ) {
		m_engine = m_apiset.os40();
	}

	// update the application list
	UpdateAppSizer();

	// if plugin can be configured in new engine, keep our current
	// config, otherwise, reset
	if( m_engine->GetConverter().IsPluginSupported(m_app_plugin->GetPluginName(*m_engine)) ) {
		// update the app list
		SelectApplication();
	}
	else {
		// leave GUI as is, and zap our plugin data
		m_app_plugin.reset();
	}
}

void GroupCfgDlg::OnAppComboChange(wxCommandEvent &event)
{
	// if the application changes, that invalidates our plugin config
	m_app_plugin.reset();
}

bool GroupCfgDlg::TransferDataFromWindow()
{
	// engine must be set!
	if( !m_engine ) {
		wxMessageBox(_T("Please select an engine."),
			_T("Device Config"), wxOK | wxICON_ERROR, this);
		return false;
	}

	// make sure the Barry plugin is configured
	if( !m_barry_plugin.IsConfigured(*m_engine) ) {
		wxMessageBox(_T("Barry doesn't have a PIN number.  This should never happen."),
			_T("Device Config"), wxOK | wxICON_ERROR, this);
		return false;
	}

	// make sure the application plugin is configured
	if( !m_app_plugin.get() || !m_app_plugin->IsConfigured(*m_engine) ) {
		// the app hasn't been configured yet, do it automatically
		wxCommandEvent event;
		OnConfigureApp(event);

		if( !m_app_plugin.get() || !m_app_plugin->IsConfigured(*m_engine) ) {
			wxMessageBox(_T("The application plugin is not fully configured."),
				_T("Application Config"), wxOK | wxICON_ERROR, this);
			return false;
		}
	}

	// copy over barry specific settings
	m_barry_plugin.SetPassword(string(m_password_edit->GetValue().utf8_str()));
	m_barry_plugin.DebugMode(m_debug_check->GetValue());
	return true;
}

int GroupCfgDlg::ShowModal()
{
	int status = wxDialog::ShowModal();
	if( status == wxID_OK && m_app_plugin.get() ) {
		// construct a new group from user's results
		m_group.reset( new Config::Group(m_group_name) );
		m_group->AddPlugin( new Config::Barry(m_barry_plugin) );
		m_group->AddPlugin( m_app_plugin->Clone() );
		m_group->DisconnectMembers(); // just to be safe
	}
	else {
		m_group.reset();
	}
	return status;
}

