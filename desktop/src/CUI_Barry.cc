///
/// \file	CUI_Barry.cc
///		ConfigUI derived class to configure the Barry "App"
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

#include "CUI_Barry.h"
#include "barrydesktop.h"
#include <iostream>
#include <sstream>

using namespace std;

namespace AppConfig {

Barry::Barry()
{
}

std::string Barry::AppName() const
{
	return OpenSync::Config::Barry::AppName();
}

bool Barry::Configure(wxWindow *parent)
{
	return false;
}

ConfigUI::plugin_ptr Barry::GetPlugin()
{
	return m_container;
}

bool Barry::RunApp(wxWindow *parent)
{
	return false;
}

void Barry::PreSyncAppInit()
{
	// nothing to do
}

bool Barry::ZapData(wxWindow *parent,
			OpenSync::Config::Group::plugin_ptr plugin,
			OpenSync::API *engine)
{
	m_parent = parent;

	// extract OpenSync::Config::Barry from plugin
	// this *can* throw an exception if the wrong plugin is
	// passed in, but we want this... such an exception would
	// represent a bug in the app, not a runtime error
	OpenSync::Config::Barry &barry =
		dynamic_cast<OpenSync::Config::Barry&>(*plugin);

	// build device name
	string device_name = barry.GetPin().str();
	const ::Barry::Probe::Results &results = wxGetApp().GetResults();
	int index = ::Barry::Probe::Find(results, barry.GetPin());
	if( index != -1 && results[index].m_cfgDeviceName.size() )
		device_name += " (" + results[index].m_cfgDeviceName + ")";

	// build intro message
	ostringstream oss;
	oss << "Please select the databases you wish to erase\n"
		"on device: " << device_name << "\n"
		"\n"
		"Note: all synced databases must be erased\n"
		"to avoid a slow-sync.";
	wxString msg(oss.str().c_str(), wxConvUTF8);

	// build list of databases (base on information from engine, if
	// the pointer is valid)
	wxArrayString dbs;
	wxArrayInt selections;
	if( !engine || engine->IsContactSyncSupported() ) {
		dbs.Add( wxString(::Barry::Contact::GetDBName(), wxConvUTF8) );
		selections.Add(dbs.GetCount() - 1);
	}
	if( !engine || engine->IsCalendarSyncSupported() ) {
		dbs.Add( wxString(::Barry::Calendar::GetDBName(), wxConvUTF8) );
		selections.Add(dbs.GetCount() - 1);
	}
	if( !engine || engine->IsMemoSyncSupported() ) {
		dbs.Add( wxString(::Barry::Memo::GetDBName(), wxConvUTF8) );
		selections.Add(dbs.GetCount() - 1);
	}
	if( !engine || engine->IsTodoSyncSupported() ) {
		dbs.Add( wxString(::Barry::Task::GetDBName(), wxConvUTF8) );
		selections.Add(dbs.GetCount() - 1);
	}

	// present the list to the user
	int count = wxGetMultipleChoices(selections, msg,
		_T("Select Databases to Erase"), dbs);
	if( count <= 0 )
		return false;	// nothing to do

	// display selections to the user for one final confirmation
	oss.str("");
	oss << "You have selected the following databases to be completely "
		"erased from device " << device_name << ":\n\n";
	for( size_t i = 0; i < selections.GetCount(); i++ ) {
		oss << string(dbs[selections[i]].utf8_str()) << "\n";
	}
	oss << "\nProceed with erase?";
	wxString confirm(oss.str().c_str(), wxConvUTF8);
	int choice = wxMessageBox(confirm, _T("Erase Confirmation"),
		wxYES_NO | wxICON_QUESTION, m_parent);
	if( choice != wxYES )
		return false;		// nothing to do

	// connect to the device and delete all selected databases
	wxBusyCursor wait;
	::Barry::Controller con(results[index]);
	::Barry::Mode::Desktop desktop(con);
	desktop.Open();
	const ::Barry::DatabaseDatabase &dbdb = desktop.GetDBDB();

	for( size_t i = 0; i < selections.GetCount(); i++ ) {

		string dbname(dbs[selections[i]].utf8_str());

		unsigned int dbid;
		if( !dbdb.GetDBNumber(dbname, dbid) ) {
			cout << "No database named '" << dbname << "' in device!" << endl;
			continue;
		}

		cout << "Clearing db: " << dbname << endl;
		desktop.ClearDatabase(dbid);
	}

	return true;
}

}

