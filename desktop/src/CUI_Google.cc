///
/// \file	CUI_Google.cc
///		ConfigUI derived class to configure the Google App
///

/*
    Copyright (C) 2010-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "CUI_Google.h"
#include <wx/wx.h>
#include <wx/process.h>
#include <gcalendar.h>
#include <sstream>
#include "wxi18n.h"
#include "i18n.h"

using namespace std;

namespace AppConfig {

//////////////////////////////////////////////////////////////////////////////
// Google config UI class

Google::Google()
	: m_google(0)
	, m_parent(0)
{
//	gcal_t google_calendar = gcal_new(GCALENDAR);
//	gcal_get_authentication(google_calendar, "username", "password");
}

std::string Google::AppName() const
{
	return OpenSync::Config::Google::AppName();
}

bool Google::Configure(wxWindow *parent, plugin_ptr old_plugin)
{
	m_parent = parent;

	// create our plugin config
	m_google = dynamic_cast<OpenSync::Config::Google*> (old_plugin.get());
	if( m_google ) {
		m_google = m_google->Clone();
	}
	else {
		m_google = new OpenSync::Config::Google;
	}
	m_container.reset( m_google );

	// display dialog and let user fill in the details
	return false; // FIXME - not finished
}

ConfigUI::plugin_ptr Google::GetPlugin()
{
	m_google = 0;
	return m_container;
}

bool Google::RunApp(wxWindow *parent)
{
	return false;
}

void Google::PreSyncAppInit()
{
}

bool Google::ZapData(wxWindow *parent,
			plugin_ptr plugin,
			OpenSync::API *engine)
{
	m_parent = parent;

	// extract OpenSync::Config::Google from plugin
	// this *can* throw an exception if the wrong plugin is
	// passed in, but we want this... such an exception would
	// represent a bug in the app, not a runtime error
	OpenSync::Config::Google &google =
		dynamic_cast<OpenSync::Config::Google&>(*plugin);

	// build intro message
	ostringstream oss;
	oss << _C("Please select the databases you wish to erase\n"
		"in your Google Calendar: \n"
		"\n"
		"Note: all synced databases must be erased\n"
		"to avoid a slow-sync.");
	wxString msg(oss.str().c_str(), wxConvUTF8);

	// build list of databases (base on information from engine, if
	// the pointer is valid)
	wxArrayString dbs;
	wxArrayInt selections;
	dbs.Add( _W("Calendar") );	selections.Add(0);
	dbs.Add( _W("Contacts") );	selections.Add(1);

	// present the list to the user
	int count = wxGetMultipleChoices(selections, msg,
		_W("Select Databases to Erase"), dbs, m_parent);
	if( count <= 0 )
		return false;	// nothing to do

	// display selections to the user for one final confirmation
	oss.str("");
	oss << _C("You have selected the following databases to be completely "
		"erased from your Google Calendar:") << "\n\n";
	for( size_t i = 0; i < selections.GetCount(); i++ ) {
		oss << string(dbs[selections[i]].utf8_str()) << "\n";
	}
	oss << "\n" << _C("Proceed with erase?");
	wxString confirm(oss.str().c_str(), wxConvUTF8);
	int choice = wxMessageBox(confirm, _W("Erase Confirmation"),
		wxYES_NO | wxICON_QUESTION, m_parent);
	if( choice != wxYES )
		return false;		// nothing to do

	// might be busy for a bit
	wxBusyCursor wait;

	// connect to Google Calendar and delete all selected databases
	(void)google;
	return false; // FIXME - not finished
}

} // namespace AppConfig

