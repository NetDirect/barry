///
/// \file	CUI_Barry.cc
///		ConfigUI derived class to configure the Barry "App"
///

/*
    Copyright (C) 2010-2013, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "wxi18n.h"

using namespace std;

namespace AppConfig {

Barry::Barry()
	: m_parent(0)
{
}

std::string Barry::AppName() const
{
	return OpenSync::Config::Barry::AppName();
}

bool Barry::Configure(wxWindow *parent, plugin_ptr old_plugin)
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
			plugin_ptr plugin,
			OpenSync::API *engine)
{
	try {

	m_parent = parent;

	// extract OpenSync::Config::Barry from plugin
	// this *can* throw an exception if the wrong plugin is
	// passed in, but we want this... such an exception would
	// represent a bug in the app, not a runtime error
	OpenSync::Config::Barry &barry =
		dynamic_cast<OpenSync::Config::Barry&>(*plugin);

	// build device name
	string device_name;
	const ::Barry::Probe::Results &results = wxGetApp().GetResults();
	int index = ::Barry::Probe::Find(results, barry.GetPin());
	if( index != -1 )
		device_name = results[index].GetDisplayName();
	else
		device_name = barry.GetPin().Str(); // default to PIN if not in list

	// build intro message
	wxString msg = wxString::Format(
		_W("Please select the databases you wish to erase\n"
		"on device: %s\n"
		"\n"
		"Note: all synced databases must be erased\n"
		"to avoid a slow-sync."),
		wxString(device_name.c_str(), wxConvUTF8).c_str());

	// build list of databases (base on information from engine, if
	// the pointer is valid)
	wxArrayString dbs;
	wxArrayInt selections;
	if( !engine || (barry.GetSupportedSyncTypes(*engine) & PST_CONTACTS) ) {
		dbs.Add( wxString(::Barry::Contact::GetDBName(), wxConvUTF8) );
		selections.Add(dbs.GetCount() - 1);
	}
	if( !engine || (barry.GetSupportedSyncTypes(*engine) & PST_EVENTS) ) {
		dbs.Add( wxString(::Barry::Calendar::GetDBName(), wxConvUTF8) );
		selections.Add(dbs.GetCount() - 1);
	}
	if( !engine || (barry.GetSupportedSyncTypes(*engine) & PST_NOTES) ) {
		dbs.Add( wxString(::Barry::Memo::GetDBName(), wxConvUTF8) );
		selections.Add(dbs.GetCount() - 1);
	}
	if( !engine || (barry.GetSupportedSyncTypes(*engine) & PST_TODOS) ) {
		dbs.Add( wxString(::Barry::Task::GetDBName(), wxConvUTF8) );
		selections.Add(dbs.GetCount() - 1);
	}

	// present the list to the user
	int count = wxGetMultipleChoices(selections, msg,
		_W("Select Databases to Erase"), dbs, m_parent);
	if( count <= 0 )
		return false;	// nothing to do

	// display selections to the user for one final confirmation
	ostringstream oss;
	oss << _C("You have selected the following databases to be completely "
		"erased from device: ") << device_name << "\n\n";
	for( size_t i = 0; i < selections.GetCount(); i++ ) {
		oss << string(dbs[selections[i]].utf8_str()) << "\n";
	}
	oss << "\n" << _C("Proceed with erase?");
	wxString confirm(oss.str().c_str(), wxConvUTF8);
	int choice = wxMessageBox(confirm, _W("Erase Confirmation"),
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
			barryverbose(_C("No database named '") << dbname << _C("' in device!"));
			continue;
		}

		barryverbose(_C("Clearing db: ") << dbname);
		desktop.ClearDatabase(dbid);
	}

	return true;

	} catch( ::Barry::Error &e ) {
		ostringstream oss;
		oss << _C("Barry exception: ") << e.what() << "\n\n"
		    << _C("You may need to do a USB reset and rescan from the "
			"main menu.");
		wxString msg(oss.str().c_str(), wxConvUTF8);
		wxMessageBox(msg, _W("Barry Exception"),
			wxOK | wxICON_ERROR, m_parent);
		return false;
	}
}

}

