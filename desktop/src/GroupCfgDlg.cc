///
/// \file	GroupCfgDlg.cc
///		The configuration dialog used when a user double clicks
///		on a device in the device list.  It lets the user choose
///		the app to sync with Barry, as well as the engine to use.
///

/*
    Copyright (C) 2009-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "barrydesktop.h"
#include <string>
#include "wxi18n.h"
#include "i18n.h"

using namespace std;
using namespace OpenSync;

BEGIN_EVENT_TABLE(GroupCfgDlg, wxDialog)
	EVT_BUTTON	(Dialog_GroupCfg_AppConfigButton,
				GroupCfgDlg::OnConfigureApp)
	EVT_CHECKBOX	(Dialog_GroupCfg_ContactsCheck,
				GroupCfgDlg::OnSyncTypeCheck)
	EVT_CHECKBOX	(Dialog_GroupCfg_EventsCheck,
				GroupCfgDlg::OnSyncTypeCheck)
	EVT_CHECKBOX	(Dialog_GroupCfg_NotesCheck,
				GroupCfgDlg::OnSyncTypeCheck)
	EVT_CHECKBOX	(Dialog_GroupCfg_TodosCheck,
				GroupCfgDlg::OnSyncTypeCheck)
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
	: wxDialog(parent, Dialog_GroupCfg, _W("Device Sync Configuration"))
	, m_device(device)
	, m_apiset(apiset)
	, m_app_count(0)
	, m_engine(0)
	, m_barry_plugin(m_device.GetPin())
	, m_topsizer(0)
	, m_appsizer(0)
	, m_engine_combo(0)
	, m_app_combo(0)
	, m_password_edit(0)
	, m_name_edit(0)
	, m_debug_check(0)
	, m_sync_contacts_check(0)
	, m_sync_events_check(0)
	, m_sync_notes_check(0)
	, m_sync_todos_check(0)
	, m_favour_radios(0)
{
	std::string appname;

	// make sure there is at least one engine
	if( !apiset.os22() && !apiset.os40() )
		throw std::logic_error(_C("Must have at least one engine in GroupCfgDlg"));

	// setup the raw GUI
	CreateLayout();

	// set window title to device PIN and name
	string label = _C("Configure Device - ");
	label += m_device.GetPin().Str();
	if( m_device.GetDeviceName().size() )
		label += " (" + m_device.GetDeviceName() + ")";
	SetTitle(wxString(label.c_str(), wxConvUTF8));

	// copy over the extras
	// and initialize the engine / sync type map with config data
	// if available
	if( m_device.GetExtras() ) {
		const DeviceExtras *extras = m_device.GetExtras();
		m_favour_plugin_name = extras->m_favour_plugin_name;

		m_sync_types[apiset.os22()] = extras->m_sync_types;
		m_sync_types[apiset.os40()] = extras->m_sync_types;
	}
	else {
		// default to all on, in worst case scenario
		m_sync_types[apiset.os22()] = PST_ALL;
		m_sync_types[apiset.os40()] = PST_ALL;
	}

	// initialize current engine pointer
	if( m_device.GetEngine() ) {
		m_engine = const_cast<OpenSync::API*> (m_device.GetEngine());
	}

	// initialize local group and plugin data
	if( m_engine && m_device.GetConfigGroup() ) {
		const Config::Group *group = m_device.GetConfigGroup();
		// use existing group name, if available
		m_group_name = group->GetGroupName();

		// copy Barry plugin config, if available
		if( group->HasBarryPlugins() )
			m_barry_plugin = group->GetBarryPlugin();

		// copy non-Barry plugin config, if available
		const Config::Plugin *plugin = group->GetNonBarryPlugin();
		if( plugin ) {
			appname = plugin->GetAppName();
			m_plugins[m_engine][appname].reset( plugin->Clone() );
		}
	}
	else {
		m_group_name = "barrydesktop_" + m_device.GetPin().Str();
	}

	SelectCurrentEngine();
	LoadBarryConfig();
	SelectApplication(appname);
	SelectFavour();

	if( m_app_count == 0 ) {
		wxMessageBox(_W("No supported applications found.  You may need to install some opensync plugins."),
			_W("No App Found"), wxOK | wxICON_ERROR, this);
	}
}

void GroupCfgDlg::CreateLayout()
{
	m_topsizer = new wxBoxSizer(wxVERTICAL);
	AddEngineSizer(m_topsizer);
	AddConfigSizer(m_topsizer);
	AddSyncTypeSizer(m_topsizer);
	AddFavourSizer(m_topsizer);
	AddButtonSizer(m_topsizer);

	SetSizer(m_topsizer);
	m_topsizer->SetSizeHints(this);
	m_topsizer->Layout();
}

void GroupCfgDlg::AddEngineSizer(wxSizer *sizer)
{
	wxSizer *engine = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _W("OpenSync Engine")),
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

	// if only one engine is available, don't bother showing the combo
	if( !m_apiset.os22() || !m_apiset.os40() ) {
		sizer->Hide(engine, true);
	}
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
		new wxStaticBox(this, wxID_ANY, _W("Barry Config")),
		wxVERTICAL
		);

	wxSizer *dname = new wxBoxSizer(wxHORIZONTAL);
	dname->Add(
		new wxStaticText(this, wxID_ANY, _W("Name:")),
		0, wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL, 2);
	dname->Add(
		m_name_edit = new wxTextCtrl(this, wxID_ANY, _T("")),
		1, wxALIGN_LEFT, 0);
	barry->Add(dname, 0, wxALL | wxEXPAND, 5);

	wxSizer *password = new wxBoxSizer(wxHORIZONTAL);
	password->Add(
		new wxStaticText(this, wxID_ANY, _W("Password:")),
		0, wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL, 2);
	password->Add(
		m_password_edit = new wxTextCtrl(this, wxID_ANY, _T(""),
			wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD),
		1, wxALIGN_LEFT, 0);
	barry->Add(password, 0, wxALL | wxEXPAND, 5);

	barry->Add(
		m_debug_check = new wxCheckBox(this, wxID_ANY,
			_W("Debug output during sync")),
		0, wxALIGN_LEFT, 5);

	sizer->Add(barry, 0, wxRIGHT | wxEXPAND, 5);
}

void GroupCfgDlg::AddAppSizer(wxSizer *sizer)
{
	m_appsizer = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _W("Application")),
		wxVERTICAL
		);

	UpdateAppSizer(false);

	sizer->Add(m_appsizer, 0, wxLEFT | wxEXPAND, 5);
}

void GroupCfgDlg::UpdateAppSizer(bool relayout)
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
		0, wxALL | wxALIGN_CENTRE, 5);
	m_appsizer->Add(
		new wxButton(this, Dialog_GroupCfg_AppConfigButton,
			_W("&Configure...")),
		0, wxALL | wxALIGN_CENTRE, 5);

	// in case this is called after the dialog is already displayed,
	// we need to readjust everything
	if( relayout )
		m_topsizer->Layout();
}

void GroupCfgDlg::LoadAppNames(wxArrayString &appnames)
{
	// need to load app names based on engine plugin availability
	// NOTE - do not load the Barry plugin, since that's already assumed

	if( !m_engine ) {
		// no engine available
		appnames.Add(_W("No engine selected"));
		return;
	}

	string_list_type plugins;
	try {
		m_engine->GetPluginNames(plugins);
	}
	catch( std::exception &e ) {
		barrylog(_C("Exception caught in LoadAppNames: ") << e.what());
		return;
	}

	// cycle through all available plugins, and add the ones
	// that we support
	int added = 0;
	string_list_type::const_iterator i = plugins.begin();
	for( ; i != plugins.end(); ++i ) {
		string appname;
		if( m_engine->GetConverter().IsPluginSupported(*i, &appname) ) {
			// found a supported plugin...

			// skip Barry
			if( appname == Config::Barry::AppName() )
				continue;

			appnames.Add( wxString(appname.c_str(), wxConvUTF8) );
			added++;
		}
	}

	m_app_count = added;

	if( m_app_count == 0 ) {
		appnames.Add(_W("No supported plugins available"));
		return;
	}
}

void GroupCfgDlg::AddSyncTypeSizer(wxSizer *sizer)
{
	wxStaticBoxSizer *checks = new wxStaticBoxSizer(wxHORIZONTAL, this,
			_W("Sync:"));

	checks->Add( m_sync_contacts_check = new wxCheckBox(this,
			Dialog_GroupCfg_ContactsCheck, _W("Contacts")),
		0, wxRIGHT | wxEXPAND, 10);
	checks->Add( m_sync_events_check = new wxCheckBox(this,
			Dialog_GroupCfg_EventsCheck, _W("Events")),
		0, wxRIGHT | wxEXPAND, 10);
	checks->Add( m_sync_notes_check = new wxCheckBox(this,
			Dialog_GroupCfg_NotesCheck, _W("Notes")),
		0, wxRIGHT | wxEXPAND, 10);
	checks->Add( m_sync_todos_check = new wxCheckBox(this,
			Dialog_GroupCfg_TodosCheck, _W("To-dos")),
		0, wxRIGHT | wxEXPAND, 10);

	sizer->Add( checks, 
		0, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 10);
}

void GroupCfgDlg::AddFavourSizer(wxSizer *sizer)
{
	wxArrayString labels;
	labels.Add( _W("Favour device") );
	labels.Add( _W("Favour application") );
	labels.Add( _W("Ask me") );

	sizer->Add( m_favour_radios = new wxRadioBox(this, wxID_ANY,
			_W("To Resolve Conflicts:"),
			wxDefaultPosition, wxDefaultSize,
			labels, 1, wxRA_SPECIFY_ROWS),
		0, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 10);
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

	// use m_device here, since BarryDesktopApp::GetDeviceName()
	// may return an empty string if the device is not currently
	// plugged in
	wxString dname(m_device.GetDeviceName().c_str(), wxConvUTF8);

	m_name_edit->SetValue(dname);
	m_password_edit->SetValue(wxString(bp.GetPassword().c_str(), wxConvUTF8));
	m_debug_check->SetValue( bp.IsDebugMode() );
}

void GroupCfgDlg::SelectApplication(const std::string appname)
{
	m_app_combo->SetValue(wxString(appname.c_str(), wxConvUTF8));
	SelectSyncTypes();
}

void GroupCfgDlg::SelectSyncTypes()
{
	if( !m_engine ) {
		SetSyncTypeChecks(PST_NONE);
		EnableSyncTypeChecks(PST_NONE);
		return;
	}

	string app = GetCurrentAppName();
	plugin_ptr ap = GetCurrentPlugin();

	// calculate the supported sync types
	// Note:   we could also take the Barry plugin config into
	//         consideration here, but so far, we just use the
	//         opensync group config
	pst_type supported = PST_NONE;
	if( ap.get() ) {
		supported = m_barry_plugin.GetSupportedSyncTypes(*m_engine)
			& ap->GetSupportedSyncTypes(*m_engine);
	}

	// make sure our current selection is limited by our new
	// set of supported plugins
	m_sync_types[m_engine] &= supported;

	// enable the checkboxes according to our ability
	EnableSyncTypeChecks(supported);

	// set the checkboxes according to our choices
	SetSyncTypeChecks(m_sync_types[m_engine]);
}

void GroupCfgDlg::SelectFavour()
{
	if( m_engine &&
	    m_favour_plugin_name == Config::Barry::PluginName(*m_engine) )
	{
		m_favour_radios->SetSelection(0);
	}
	else if( m_engine && m_favour_plugin_name.size() ) {
		m_favour_radios->SetSelection(1);
	}
	else {
		// ask the user
		m_favour_radios->SetSelection(2);
	}
}

void GroupCfgDlg::EnableSyncTypeChecks(pst_type types)
{
	m_sync_contacts_check->Enable( types & PST_CONTACTS );
	m_sync_events_check  ->Enable( types & PST_EVENTS );
	m_sync_notes_check   ->Enable( types & PST_NOTES );
	m_sync_todos_check   ->Enable( types & PST_TODOS );
}

void GroupCfgDlg::SetSyncTypeChecks(pst_type types)
{
	m_sync_contacts_check->SetValue( types & PST_CONTACTS );
	m_sync_events_check  ->SetValue( types & PST_EVENTS );
	m_sync_notes_check   ->SetValue( types & PST_NOTES );
	m_sync_todos_check   ->SetValue( types & PST_TODOS );
}

GroupCfgDlg::pst_type GroupCfgDlg::GetSyncTypeChecks()
{
	pst_type types = PST_NONE;
	if( m_sync_contacts_check->GetValue() ) types |= PST_CONTACTS;
	if( m_sync_events_check  ->GetValue() ) types |= PST_EVENTS;
	if( m_sync_notes_check   ->GetValue() ) types |= PST_NOTES;
	if( m_sync_todos_check   ->GetValue() ) types |= PST_TODOS;
	return types;
}

std::string GroupCfgDlg::GetCurrentAppName() const
{
	wxString app = m_app_combo->GetValue();
	return std::string(app.utf8_str());
}

GroupCfgDlg::plugin_ptr GroupCfgDlg::GetCurrentPlugin()
{
	string appname = GetCurrentAppName();
	appcfg_map &cfgs = m_plugins[m_engine];
	appcfg_map::iterator pi = cfgs.find(appname);
	if( pi != cfgs.end() )
		return pi->second;
	else
		return plugin_ptr();	// not found, return empty ptr
}

void GroupCfgDlg::OnConfigureApp(wxCommandEvent &event)
{
	string app = GetCurrentAppName();
	if( app.size() == 0 ) {
		wxMessageBox(_W("Please select an application."),
			_W("Application Config"), wxOK | wxICON_ERROR, this);
		return;
	}

	ConfigUI::ptr ui = ConfigUI::CreateConfigUI(app);

	if( !ui.get() ) {
		wxMessageBox(_W("No configuration interface available for this Application."),
			_W("Application Config"),
			wxOK | wxICON_ERROR, this);
		return;
	}

	if( ui->Configure(this, GetCurrentPlugin()) ) {
		ConfigUI::plugin_ptr plugin = ui->GetPlugin();
		if( plugin.get() ) {
			m_plugins[m_engine][app] = plugin;

			// if this is the first time, default to all
			if( m_sync_types[m_engine] == PST_NONE )
				m_sync_types[m_engine] = PST_ALL;

			// update the types checkboxes
			SelectSyncTypes();
		}
	}
}

void GroupCfgDlg::OnEngineComboChange(wxCommandEvent &event)
{
	// remember what plugin we're using
	plugin_ptr old_app = GetCurrentPlugin();

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
	if( old_app.get() && m_engine->GetConverter().IsPluginSupported(old_app->GetPluginName(*m_engine)) ) {
		// update the app list
		SelectApplication(old_app->GetAppName());
	}
	else {
		// leave GUI as is, and zap our plugin data
		SelectApplication("");
	}
}

void GroupCfgDlg::OnAppComboChange(wxCommandEvent &event)
{
	SelectSyncTypes();
}

void GroupCfgDlg::OnSyncTypeCheck(wxCommandEvent &event)
{
	if( !m_engine )
		return;

	m_sync_types[m_engine] = GetSyncTypeChecks();
}

bool GroupCfgDlg::TransferDataFromWindow()
{
	// engine must be set!
	if( !m_engine ) {
		wxMessageBox(_W("Please select an engine."),
			_W("Device Config"), wxOK | wxICON_ERROR, this);
		return false;
	}

	// make sure the Barry plugin is configured
	if( !m_barry_plugin.IsConfigured(*m_engine) ) {
		wxMessageBox(_W("Barry doesn't have a PIN number.  This should never happen."),
			_W("Device Config"), wxOK | wxICON_ERROR, this);
		return false;
	}

	// make sure the application plugin is configured
	plugin_ptr app = GetCurrentPlugin();
	if( !app.get() || !app->IsConfigured(*m_engine) ) {
		// the app hasn't been configured yet, do it automatically
		wxCommandEvent event;
		OnConfigureApp(event);

		app = GetCurrentPlugin();
		if( !app.get() || !app->IsConfigured(*m_engine) ) {
			wxMessageBox(_W("The application plugin is not fully configured."),
				_W("Application Config"), wxOK | wxICON_ERROR, this);
			return false;
		}
	}

	// copy over barry specific settings
	m_device_name = string(m_name_edit->GetValue().utf8_str());
	m_barry_plugin.SetPassword(string(m_password_edit->GetValue().utf8_str()));
	m_barry_plugin.DebugMode(m_debug_check->GetValue());

	// make sure conflict resolution is known
	int findex = m_favour_radios->GetSelection();
	switch( findex )
	{
	case 0:	// Favour device
		m_favour_plugin_name = Config::Barry::PluginName(*m_engine);
		break;

	case 1:	// Favour application
		m_favour_plugin_name = app->GetPluginName(*m_engine);
		break;

	case 2:	// Ask me
		m_favour_plugin_name.clear();
		break;

	default: // borked
		wxMessageBox(_W("Please select conflict resolution method."),
			_W("Conflict Resolution"), wxOK | wxICON_ERROR, this);
		return false;
	}

	// save the new device name
	wxGetApp().SetDeviceName(m_barry_plugin.GetPin(), m_device_name);

	// save the sync type checkboxes
	m_sync_types[m_engine] = GetSyncTypeChecks();

	return true;
}

int GroupCfgDlg::ShowModal()
{
	int status = wxDialog::ShowModal();
	plugin_ptr app = GetCurrentPlugin();
	if( status == wxID_OK && app.get() ) {
		// construct a new group from user's results
		m_group.reset( new Config::Group(m_group_name) );
		m_group->AddPlugin( m_barry_plugin.Clone() );
		m_group->AddPlugin( app->Clone() );

		// don't forget the extras
		m_extras.reset( new DeviceExtras(m_barry_plugin.GetPin()) );
		m_extras->m_favour_plugin_name = m_favour_plugin_name;
		m_extras->m_sync_types = m_sync_types[m_engine];
	}
	else {
		m_group.reset();
		m_extras.reset();
	}
	return status;
}

