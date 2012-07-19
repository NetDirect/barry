///
/// \file	Mode_Sync.cc
///		Mode derived class for syncing
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

#include "Mode_Sync.h"
#include "BaseFrame.h"
#include "GroupCfgDlg.h"
#include "SyncStatusDlg.h"
#include "barrydesktop.h"
#include "windowids.h"
#include <string>

using namespace std;

BEGIN_EVENT_TABLE(SyncMode, wxEvtHandler)
	EVT_BUTTON	(SyncMode_SyncNowButton, SyncMode::OnSyncNow)
	EVT_BUTTON	(SyncMode_ConfigureButton, SyncMode::OnConfigure)
	EVT_BUTTON	(SyncMode_RunAppButton, SyncMode::OnRunApp)
	EVT_BUTTON	(SyncMode_1WayResetButton, SyncMode::On1WayReset)
	EVT_LIST_ITEM_SELECTED(SyncMode_DeviceList, SyncMode::OnListSelChange)
	EVT_LIST_ITEM_DESELECTED(SyncMode_DeviceList, SyncMode::OnListSelChange)
	EVT_LIST_ITEM_ACTIVATED(SyncMode_DeviceList, SyncMode::OnConfigureDevice)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////////
// SyncMode

SyncMode::SyncMode(wxWindow *parent)
	: m_parent(parent)
{
	wxBusyCursor wait;

	wxSize client_size = parent->GetClientSize();

	// create our list of devices
	m_device_set.reset( new DeviceSet(wxGetApp().GetGlobalConfig(),
					wxGetApp().GetOpenSync(),
					wxGetApp().GetResults()) );
	barryverbose(*m_device_set);

	// eliminate all duplicate device entries
	DeviceSet::subset_type subset;
	do {
		subset = m_device_set->FindDuplicates();
		if( subset.size() ) {
			// build list of choices
			wxArrayString choices;
			DeviceSet::subset_type::iterator i = subset.begin();
			for( ; i != subset.end(); ++i ) {
				string desc = (*i)->GetIdentifyingString();
				choices.Add( wxString(desc.c_str(), wxConvUTF8) );
			}

			// let the user choose
			// FIXME - the width of the choice dialog is
			// determined by the length of the string...
			// which is less than ideal
			int choice = wxGetSingleChoiceIndex(_W("Multiple configurations have been found with the same PIN.  Please select\nthe configuration that Barry Desktop should work with."),
				_T("Duplicate PIN"),
				choices, parent);

			// remove everything except keep
			if( choice != -1 ) {
				subset.erase(subset.begin() + choice);
			}

			m_device_set->KillDuplicates(subset);

			barryverbose(*m_device_set);
		}
	} while( subset.size() );

	//
	// create the window controls we need
	//

	m_topsizer.reset( new wxBoxSizer(wxVERTICAL) );

	// make space for the main header, which is not part of our
	// work area
	m_topsizer->AddSpacer(MAIN_HEADER_OFFSET);

	// add status area
	//
	// Select the device(s) you want to sync and press Sync Now
	// Press Configure... to configure the currently selected device
	// Press Run App to start the application that the device syncs with
	// Press 1-Way Reset to recover from a broken sync, copying all
	//        device data to application, or vice versa.
	//
	m_topsizer->AddSpacer(5);

	m_sync_now_button.reset( new wxButton(parent,
				SyncMode_SyncNowButton, _T("Sync Now")));
	wxSize sync_button_size = m_sync_now_button->GetClientSize();
	int wrapwidth = client_size.GetWidth() - 20 - sync_button_size.GetWidth();

	wxBoxSizer *infosizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *linesizer = new wxBoxSizer(wxVERTICAL);

	// info lines
#define MAKE_INFO_LABEL(a, b) \
	m_label[a].reset( new wxStaticText(parent, -1, b, \
		wxPoint(15, 100)) ); \
	m_label[a]->Wrap(wrapwidth); \
	linesizer->Add(m_label[a].get(), 0, wxEXPAND, 0); \
	linesizer->AddSpacer(4);
	MAKE_INFO_LABEL(0, _W("Select the device(s) you want to sync and press Sync Now."));
	MAKE_INFO_LABEL(1, _W("Use Configure to configure the currently selected device."));
	MAKE_INFO_LABEL(2, _W("Use Run App to start the application that the device syncs with."));
	MAKE_INFO_LABEL(3, _W("Use 1-Way Reset to recover from a broken sync, copying all device data to application, or vice versa."));

	infosizer->Add( linesizer, 1, wxALIGN_LEFT, 0 );
	infosizer->Add( m_sync_now_button.get(), 0, wxALIGN_RIGHT, 0 );
	m_topsizer->Add( infosizer,
				0, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, 10 );

	// status, final spacing
	m_topsizer->AddSpacer(10);

	// add device list
	wxStaticBoxSizer *box = new wxStaticBoxSizer(wxHORIZONTAL, parent,
		_W("Device List"));
	m_device_list.reset (new wxListCtrl(parent, SyncMode_DeviceList,
				wxDefaultPosition, wxDefaultSize,
				wxLC_REPORT /*| wxLC_VRULES*/) );
	box->Add( m_device_list.get(), 1, wxEXPAND | wxALL, 4 );
	m_topsizer->Add(box, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10 );

	// add bottom buttons - these go in the bottom FOOTER area
	// so their heights must be fixed to MAIN_HEADER_OFFSET
	// minus a border of 5px top and bottom
	wxSize footer(-1, MAIN_HEADER_OFFSET - 5 - 5);
	wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
	m_run_app_button.reset( new wxButton(parent,
				SyncMode_RunAppButton, _W("Run App"),
				wxDefaultPosition, footer));
	m_configure_button.reset( new wxButton(parent,
				SyncMode_ConfigureButton, _W("Configure..."),
				wxDefaultPosition, footer) );
	m_1way_reset_button.reset( new wxButton(parent,
				SyncMode_1WayResetButton, _W("1 Way Reset..."),
				wxDefaultPosition, footer) );
	buttons->Add(m_run_app_button.get(), 0, wxRIGHT, 5 );
	buttons->Add(m_configure_button.get(), 0, wxRIGHT, 5 );
	buttons->Add(m_1way_reset_button.get(), 0, wxRIGHT, 5 );
	m_topsizer->Add(buttons, 0, wxALL | wxALIGN_RIGHT, 5 );

	// recalc size of children
	m_topsizer->SetDimension(0, 0,
		client_size.GetWidth(), client_size.GetHeight());

	// insert list columns based on the new list size
	wxSize list_size = m_device_list->GetSize();
	int timestamp_width = GetMaxTimestampWidth(m_device_list.get());
	// FIXME - for some reason, even with the width calculated,
	// when displayed in the listctrl, there's not enough space...
	// possibly because there's space between columns... I don't
	// know how to calculate the column inter-space size, so add
	// a constant here :-(
	timestamp_width += 8;
	int usable_width = list_size.GetWidth() - timestamp_width;
	m_device_list->InsertColumn(0, _W("PIN"),
		wxLIST_FORMAT_LEFT, usable_width * 0.16);
	m_device_list->InsertColumn(1, _W("Name"),
		wxLIST_FORMAT_LEFT, usable_width * 0.33);
	m_device_list->InsertColumn(2, _W("Connected"),
		wxLIST_FORMAT_CENTRE, usable_width * 0.16);
	m_device_list->InsertColumn(3, _W("Application"),
		wxLIST_FORMAT_CENTRE, usable_width * 0.18);
	m_device_list->InsertColumn(4, _W("Engine"),
		wxLIST_FORMAT_CENTRE, usable_width * 0.17);
	m_device_list->InsertColumn(5, _W("Last Sync"),
		wxLIST_FORMAT_CENTRE, timestamp_width);

	FillDeviceList();

	// attempt to re-select the devices as we last saw them
	ReselectDevices(m_device_set->String2Subset(wxGetApp().GetGlobalConfig().GetKey("SelectedDevices")));
	UpdateButtons();

	// connect ourselves to the parent's event handling chain
	// do this last, so that we are guaranteed our destructor
	// will run, in case of exceptions
	m_parent->PushEventHandler(this);
}

SyncMode::~SyncMode()
{
	m_parent->PopEventHandler();

	// save selected devices for later
	wxGetApp().GetGlobalConfig().SetKey("SelectedDevices",
		DeviceSet::Subset2String(GetSelectedDevices()));
}

std::string SyncMode::Timestamp(time_t last_sync)
{
	string ret;
	struct tm local;
	if( localtime_r(&last_sync, &local) != NULL ) {
		char timestamp[20];
		strftime(timestamp, sizeof(timestamp), "%b %d, %H:%M", &local);
		ret = timestamp;
	}
	return ret;
}

int SyncMode::GetMaxTimestampWidth(wxWindow *win)
{
	int max_width = 0;
	DeviceSet::const_iterator i = m_device_set->begin();
	for( ; i != m_device_set->end(); ++i ) {
		int this_width = 0;
		int this_height = 0;
		if( i->GetExtras() ) {
			time_t last_sync = i->GetExtras()->m_last_sync_time;
			if( last_sync ) {
				win->GetTextExtent(wxString(Timestamp(last_sync).c_str(), wxConvUTF8), &this_width, &this_height);
			}
		}

		max_width = max(max_width, this_width);
	}

	return max_width;
}

void SyncMode::FillDeviceList()
{
	// start fresh
	m_device_list->DeleteAllItems();

	DeviceSet::const_iterator i = m_device_set->begin();
	for( int index = 0; i != m_device_set->end(); ++i, index++ ) {
		// PIN number
		wxString text(i->GetPin().Str().c_str(), wxConvUTF8);
		long item = m_device_list->InsertItem(index, text);

		// Device name
		text = wxString(i->GetDeviceName().c_str(), wxConvUTF8);
		m_device_list->SetItem(item, 1, text);

		// Connected?
		text = i->IsConnected() ? _W("Yes") : _W("No");
		m_device_list->SetItem(item, 2, text);

		// Configured?
		if( i->IsConfigured() ) {
			text = wxString(i->GetAppNames().c_str(), wxConvUTF8);
		}
		else {
			text = _W("(No config)");
		}
		m_device_list->SetItem(item, 3, text);

		// Engine
		if( i->GetEngine() )
			text = wxString(i->GetEngine()->GetVersion(), wxConvUTF8);
		else
			text = _T("");
		m_device_list->SetItem(item, 4, text);

		// Last Sync
		if( i->GetExtras() ) {
			time_t last_sync = i->GetExtras()->m_last_sync_time;
			if( last_sync ) {
				wxString ts(Timestamp(last_sync).c_str(), wxConvUTF8);
				m_device_list->SetItem(item, 5, ts);
			}
		}
	}

	UpdateButtons();
}

void SyncMode::UpdateButtons()
{
	int selected_count = m_device_list->GetSelectedItemCount();

	// update the SyncNow button (only on if anything is selected)
	m_sync_now_button->Enable(selected_count > 0);
	m_configure_button->Enable(selected_count == 1);
	m_run_app_button->Enable(selected_count == 1);
	m_1way_reset_button->Enable(selected_count == 1);

	// if only one item is selected, enable RunApp button
	bool enable_run_app = false;
	if( selected_count == 1 ) {
		// good, only one is selected, find out which one
		long item = -1;
		item = m_device_list->GetNextItem(item, wxLIST_NEXT_ALL,
			wxLIST_STATE_SELECTED);

		if( item != -1 ) {
			DeviceEntry &entry = (*m_device_set)[item];
			if( entry.GetAppNames().size() ) {
				// has application configured!
				enable_run_app = true;
			}
		}
	}
	m_run_app_button->Enable(enable_run_app);
}

DeviceSet::subset_type SyncMode::GetSelectedDevices()
{
	DeviceSet::subset_type subset;

	long item = -1;
	do {
		item = m_device_list->GetNextItem(item, wxLIST_NEXT_ALL,
			wxLIST_STATE_SELECTED);

		if( item != -1 ) {
			subset.push_back(m_device_set->begin() + item);
		}
	} while( item != -1 );

	return subset;
}

void SyncMode::ReselectDevices(const DeviceSet::subset_type &set)
{
	for( long item = m_device_list->GetNextItem(-1); item != -1;
		item = m_device_list->GetNextItem(item) )
	{
		bool selected = DeviceSet::FindPin(set, (*m_device_set)[item].GetPin()) != set.end();

		m_device_list->SetItemState(item,
			selected ? wxLIST_STATE_SELECTED : 0,
			wxLIST_STATE_SELECTED);
	}
}

void SyncMode::ConfigureDevice(int device_index)
{
	DeviceEntry &entry = (*m_device_set)[device_index];
	ConfigureDevice(entry);
}

void SyncMode::ConfigureDevice(DeviceEntry &entry)
{
	// make sure it's not already running
	if( m_cui.get() && m_cui->IsAppRunning() ) {
		wxMessageBox(_W("An application is currently running."),
			_W("Run App Error"), wxOK | wxICON_ERROR);
		return;
	}

	GroupCfgDlg dlg(m_parent, entry, wxGetApp().GetOpenSync());
	if( dlg.ShowModal() == wxID_OK &&
	    dlg.GetEngine() &&
	    dlg.GetGroup().get() &&
	    dlg.GetExtras().get() )
	{
		bool skip_rewrite = false;
		bool delete_old = false;

		// does old group exist?
		if( entry.GetEngine() &&
		    entry.GetConfigGroup() &&
		    entry.GetConfigGroup()->GroupExists(*entry.GetEngine()) )
		{
			// yes, is the new group equal?
			string v1 = entry.GetEngine()->GetVersion();
			string v2 = dlg.GetEngine()->GetVersion();
			skip_rewrite = (v1 == v2 &&
			      dlg.GetGroup()->Compare(*entry.GetConfigGroup()));

			if( skip_rewrite )  {
				// config is the same, don't bother saving again
				barryverbose(_C("Config is the same, skipping save"));
			}
			else {
				// clean up after ourselves... if the new
				// config uses a different engine, delete
				// the config on the old engine
				if( entry.GetEngine() != dlg.GetEngine() ) {
					delete_old = true;
				}
			}
		}

		if( !skip_rewrite ) try {

			OpenSync::API *eng = dlg.GetEngine();
			DeviceEntry::group_ptr grp = dlg.GetGroup();

			// make sure that the save will be successful
			if( grp->GroupExists(*eng) ) {
				if( !grp->GroupMatchesExistingConfig(*eng) ) {
					if( WarnAbout1WayReset() ) {
						eng->DeleteGroup(grp->GetGroupName());
						grp->DisconnectMembers();
					}
					else {
						// skip save
						return;
					}
				}
				else {
					// group we want to save has the
					// same set of plugins and member IDs
					// as the one already there...
					// so do not disconnect members
				}
			}
			else {
				// we are saving a brand new group, so make
				// sure that all members are new
				grp->DisconnectMembers();
			}

			// save the new one
			grp->Save(*dlg.GetEngine());

			// clean up the old engine's group, so we don't leave
			// garbage behind... do this after a successful
			// save, so that we don't delete existing knowledge
			// before we've crossed over
			if( delete_old ) {
				barryverbose("Engine change detected in config: deleting old config '" << entry.GetConfigGroup()->GetGroupName() << "' from engine " << entry.GetEngine()->GetVersion() << " in order to save it to engine " << dlg.GetEngine()->GetVersion());
				entry.GetEngine()->DeleteGroup(entry.GetConfigGroup()->GetGroupName());
			}

		}
		catch( OpenSync::Config::SaveError &se ) {
			barryverbose(_C("Exception during save: ") << se.what());
			wxString msg = _W("Unable to save configuration for this device.\nError: ");
			msg += wxString(se.what(), wxConvUTF8);
			wxMessageBox(msg, _W("OpenSync Save Error"),
				wxOK | wxICON_ERROR);
			return;
		}

		// save the extras... this is cheap, so no need to check
		// skip_rewrite
		dlg.GetExtras()->Save(wxGetApp().GetGlobalConfig(),
					dlg.GetGroup()->GetGroupName());

		// update the device set
		entry.SetConfigGroup(dlg.GetGroup(), dlg.GetEngine(),
			dlg.GetExtras());
		entry.SetDeviceName(dlg.GetDeviceName());

		// update!
		RefillList();
	}
}

void SyncMode::CheckConfigured(DeviceSet::subset_type &subset)
{
	for( DeviceSet::subset_type::iterator i = subset.begin();
	     i != subset.end();
	     ++i )
	{
		DeviceEntry &device = *(*i);
		if( !device.IsConnected() )
			continue;
		if( device.IsConfigured() )
			continue;

		ostringstream msg;
		msg << _C("Selected device ") << device.GetPin().Str()
			<< " (" << device.GetDeviceName() << ")"
			<< _C(" is not yet configured.  Configure now?");

		int response = wxMessageBox(
			wxString(msg.str().c_str(),wxConvUTF8),
			_W("Configure Now?"), wxYES_NO, m_parent);
		if( response == wxYES ) {
			ConfigureDevice(device);
		}
	}
}

void SyncMode::RefillList()
{
	DeviceSet::subset_type subset = GetSelectedDevices();
	FillDeviceList();
	ReselectDevices(subset);
}

int SyncMode::GetSelectedDevice()
{
	if( m_device_list->GetSelectedItemCount() != 1 ) {
		wxMessageBox(_W("Please select one device from the list."),
			_W("Device List"), wxOK | wxICON_ERROR);
		return -1;
	}

	// find selected device
	long item = -1;
	item = m_device_list->GetNextItem(item, wxLIST_NEXT_ALL,
		wxLIST_STATE_SELECTED);
	return item;
}

//
// Returns an index into the config group for the given device index
// that represents the authoritative side of the sync.  This means
// that during the 1-Way Reset, the plugin at this index must NOT
// be zapped!  All the others must be zapped.
//
// Returns -1 if the user cancels, or if not enough data to decide.
//
int SyncMode::GetAuthoritativeSide(int device_index)
{
	// grab the device entry and group config
	DeviceEntry &entry = (*m_device_set)[device_index];
	OpenSync::Config::Group *group = entry.GetConfigGroup();
	if( !group )
		return -1;

	// build message
	wxString intro(_W(
		"Which device / application should be considered\n"
		"authoritative?\n"
		"\n"
		"All data in non-authoritative devices / applications\n"
		"will be deleted in order to setup a straight copy on\n"
		"the next sync."));

	// build list of devices / applications
	wxArrayString list;
	OpenSync::Config::Group::iterator gi = group->begin();
	for( ; gi != group->end(); ++gi ) {
		// the Barry plugin is special
		if( dynamic_cast<OpenSync::Config::Barry*>( (*gi).get() ) ) {
			// this is a Barry plugin, so display the
			// device name, not the plugin name
			string device_name = entry.GetPin().Str();
			if( entry.GetDeviceName().size() )
				device_name += " (" + entry.GetDeviceName() + ")";

			list.Add( wxString(device_name.c_str(), wxConvUTF8) );
		}
		else {
			// add the application name
			list.Add( wxString((*gi)->GetAppName().c_str(),
						wxConvUTF8) );
		}
	}

	// ask the user
	int choice = wxGetSingleChoiceIndex(intro,
		_W("Select Authoritative Device / Application"),
		list, m_parent);
	return choice;
}

bool SyncMode::ZapConflicts(int device_index, int authoritative_side)
{
	// grab the device entry and group config
	DeviceEntry &entry = (*m_device_set)[device_index];
	OpenSync::Config::Group *group = entry.GetConfigGroup();
	if( !group )
		return false;

	// cycle through list of sync plugins, zapping each
	// non-authoritative one
	OpenSync::Config::Group::iterator gi = group->begin();
	for( int i = 0; gi != group->end(); ++gi, ++i ) {

		// skip the authoritative plugin!
		if( i == authoritative_side )
			continue;

		OpenSync::Config::Plugin &plugin = *(*gi);

		// create a configUI object, for zapping
		ConfigUI::ptr ui = ConfigUI::CreateConfigUI(plugin.GetAppName());
		if( !ui.get() ) {
			// no possibility for zapping here... so just
			// assume that it doesn't need it for now...
			// worst that can happen, should be it slow-syncs
			// all the time
			continue;
		}

		bool success = ui->ZapData(m_parent, *gi, entry.GetEngine());
		if( !success ) {
			// if the user cancels one, cancel all the rest
			return false;
		}
	}

	// if we reach this, we succeeded
	return true;
}

void SyncMode::RewriteConfig(int device_index)
{
	// grab the device entry and group config
	DeviceEntry &entry = (*m_device_set)[device_index];
	OpenSync::Config::Group *group = entry.GetConfigGroup();
	OpenSync::API *engine = entry.GetEngine();
	if( !group || !engine )
		return;

	string group_name = group->GetGroupName();

	try {
		// delete the existing group name
		engine->DeleteGroup(group_name);

		// disconnect the plugins from the group, to
		// make them "new" again
		group->DisconnectMembers();

		// save
		group->Save(*engine);

		// success!
		barryverbose(group_name << " group config rewritten");
		return;
	}
	catch( std::runtime_error &re ) {
		ostringstream oss;
		oss << _C("Unable to rewrite config!  Start over manually. "
			"Error: ") << re.what();
		wxString msg(oss.str().c_str(), wxConvUTF8);

		wxMessageBox(msg, _W("Error Rewriting Config"),
			wxOK | wxICON_ERROR, m_parent);
	}

	// if we get here, the group is in an undefined state,
	// so delete it to make sure nothing odd is left behind
	try {
		engine->DeleteGroup(group_name);
	}
	catch( std::runtime_error &re ) {
		barryverbose("Error while deleting undefined group '" << group_name << "': " << re.what());
	}
}

bool SyncMode::WarnAbout1WayReset()
{
	int answer = wxMessageBox( _W("The sync config you are about to save "
		"is sufficiently different from the existing one that "
		"a 1-Way Reset will be required.  You will need to "
		"perform the reset at your earliest convenience, before "
		"your next sync.\n\n"
		"Continue anyway?"),
		_W("1-Way Reset Warning"),
		wxYES_NO | wxICON_QUESTION, m_parent);
	return answer == wxYES;
}

void SyncMode::OnSyncNow(wxCommandEvent &event)
{
	DeviceSet::subset_type subset = GetSelectedDevices();
	if( subset.size() == 0 )
		return;	// nothing to do

	// make sure an app is not running
	if( m_cui.get() && m_cui->IsAppRunning() ) {
		wxMessageBox(_W("An application is currently running."),
			_W("Sync Error"), wxOK | wxICON_ERROR);
		return;
	}

	// check that all selections are configured
	CheckConfigured(subset);

	// do the sync
	SyncStatusDlg dlg(m_parent, subset);
	dlg.ShowModal();

	// update the timestamps
	RefillList();
}

void SyncMode::OnConfigure(wxCommandEvent &event)
{
	int item = GetSelectedDevice();
	if( item != -1 )
		ConfigureDevice(item);
}

void SyncMode::OnRunApp(wxCommandEvent &event)
{
	// make sure it's not already running
	if( m_cui.get() && m_cui->IsAppRunning() ) {
		wxMessageBox(_W("An application is already running."),
			_W("Run App Error"), wxOK | wxICON_ERROR);
		return;
	}

	// find selected device
	int item = GetSelectedDevice();
	if( item == -1 )
		return;

	// retrieve device's group config
	DeviceEntry &entry = (*m_device_set)[item];
	OpenSync::Config::Plugin *plugin = 0;
	if( entry.GetConfigGroup() )
		plugin = entry.GetConfigGroup()->GetNonBarryPlugin();
	if( !plugin )
		return;

	// run the app
	m_cui = ConfigUI::CreateConfigUI(plugin->GetAppName());
	if( m_cui.get() )
		m_cui->RunApp(m_parent);
}

void SyncMode::On1WayReset(wxCommandEvent &event)
{
	int item = GetSelectedDevice();
	if( item == -1 )
		return;

	// let user pick the authoritative side of the sync
	int side = GetAuthoritativeSide(item);
	if( side == -1 )
		return;

	// zap the data of all remaining sync elements
	if( !ZapConflicts(item, side) )
		return;

	// rewrite the config
	RewriteConfig(item);

	// reload the deviceset
	RefillList();

	// tell the user all's well
	wxMessageBox(_W("1-Way Reset is complete, and ready to sync."),
		_W("Reset Complete"), wxOK | wxICON_INFORMATION, m_parent);
}

void SyncMode::OnListSelChange(wxListEvent &event)
{
	UpdateButtons();
}

void SyncMode::OnConfigureDevice(wxListEvent &event)
{
	ConfigureDevice(event.GetIndex());
}

