///
/// \file	Mode_Sync.cc
///		Mode derived class for syncing
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
	m_device_set.reset( new DeviceSet(wxGetApp().GetResults(),
		wxGetApp().GetOpenSync()) );
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
			int choice = wxGetSingleChoiceIndex(_T("Multiple configurations have been found with the same PIN.  Please select\nthe configuration that Barry Desktop should work with."),
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
	m_topsizer->AddSpacer(5);
	m_sync_now_button.reset( new wxButton(parent,
				SyncMode_SyncNowButton, _T("Sync Now")));
	m_topsizer->Add( m_sync_now_button.get(),
				0, wxRIGHT | wxALIGN_RIGHT, 10 );
	m_topsizer->AddSpacer(90);
//	m_label.reset( new wxStaticText(parent, -1, _T("Static Text"),
//		wxPoint(15, 100)) );

	// add device list
	wxStaticBoxSizer *box = new wxStaticBoxSizer(wxHORIZONTAL, parent,
		_T("Device List"));
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
				SyncMode_RunAppButton, _T("Run App"),
				wxDefaultPosition, footer));
	m_configure_button.reset( new wxButton(parent,
				SyncMode_ConfigureButton, _T("Configure..."),
				wxDefaultPosition, footer) );
	m_1way_reset_button.reset( new wxButton(parent,
				SyncMode_1WayResetButton, _T("1 Way Reset..."),
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
	m_device_list->InsertColumn(0, _T("PIN"),
		wxLIST_FORMAT_LEFT, list_size.GetWidth() * 0.16);
	m_device_list->InsertColumn(1, _T("Name"),
		wxLIST_FORMAT_LEFT, list_size.GetWidth() * 0.33);
	m_device_list->InsertColumn(2, _T("Connected"),
		wxLIST_FORMAT_CENTRE, list_size.GetWidth() * 0.16);
	m_device_list->InsertColumn(3, _T("Application"),
		wxLIST_FORMAT_CENTRE, list_size.GetWidth() * 0.18);
	m_device_list->InsertColumn(4, _T("Engine"),
		wxLIST_FORMAT_CENTRE, list_size.GetWidth() * 0.17);

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

void SyncMode::FillDeviceList()
{
	// start fresh
	m_device_list->DeleteAllItems();

	DeviceSet::const_iterator i = m_device_set->begin();
	for( int index = 0; i != m_device_set->end(); ++i, index++ ) {
		wxString text(i->GetPin().str().c_str(), wxConvUTF8);
		long item = m_device_list->InsertItem(index, text);

		text = wxString(i->GetDeviceName().c_str(), wxConvUTF8);
		m_device_list->SetItem(item, 1, text);

		text = i->IsConnected() ? _T("Yes") : _T("No");
		m_device_list->SetItem(item, 2, text);

		if( i->IsConfigured() ) {
			text = wxString(i->GetAppNames().c_str(), wxConvUTF8);
		}
		else {
			text = _T("(unconfigured)");
		}
		m_device_list->SetItem(item, 3, text);

		if( i->GetEngine() )
			text = wxString(i->GetEngine()->GetVersion(), wxConvUTF8);
		else
			text = _T("");
		m_device_list->SetItem(item, 4, text);
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
	// make sure it's not already running
	if( m_cui.get() && m_cui->IsAppRunning() ) {
		wxMessageBox(_T("An application is currently running."),
			_T("Run App Error"), wxOK | wxICON_ERROR);
		return;
	}

	DeviceEntry &entry = (*m_device_set)[device_index];

	GroupCfgDlg dlg(m_parent, entry, wxGetApp().GetOpenSync());
	if( dlg.ShowModal() == wxID_OK &&
	    dlg.GetEngine() &&
	    dlg.GetGroup().get() )
	{
		// does old group exist?
		if( entry.GetEngine() &&
		    entry.GetConfigGroup() &&
		    entry.GetConfigGroup()->GroupExists(*entry.GetEngine()) )
		{
			// yes, is the new group equal?
			string v1 = entry.GetEngine()->GetVersion();
			string v2 = dlg.GetEngine()->GetVersion();
			if( v1 == v2 && dlg.GetGroup()->Compare(*entry.GetConfigGroup()) ) {
				// config is the same, don't bother saving again
				cout << "Config is the same, skipping save" << endl;
				return;
			}

			// group config changed, delete to start over,
			// since new config might change engines...
			// and we don't want to leave stuff behind
			entry.GetEngine()->DeleteGroup(entry.GetConfigGroup()->GetGroupName());
		}

		for( int attempt = 0; attempt < 2; attempt++ ) {
			try {

				// save the new one
				dlg.GetGroup()->Save(*dlg.GetEngine());

			}
			catch( OpenSync::Config::SaveError &se ) {
				cout << "Exception during save: " << se.what() << endl;
				if( attempt < 2 ) {
					cout << "Deleting group using alternate engine and resaving: " << entry.GetConfigGroup()->GetGroupName() << endl;
					// delete and try again
					dlg.GetEngine()->DeleteGroup(entry.GetConfigGroup()->GetGroupName());
				}
				else {
					wxString msg = _T("Unable to save configuration for this device.\nError: ");
					msg += wxString(se.what(), wxConvUTF8);
					wxMessageBox(msg, _T("OpenSync Save Error"), wxOK | wxICON_ERROR);
					return;
				}
			}
		}

		// update the device set
		(*m_device_set)[device_index].
			SetConfigGroup(dlg.GetGroup(), dlg.GetEngine());

		// update!
		RefillList();
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
		wxMessageBox(_T("Please select one device from the list."),
			_T("Device List"), wxOK | wxICON_ERROR);
		return -1;
	}

	// find selected device
	long item = -1;
	item = m_device_list->GetNextItem(item, wxLIST_NEXT_ALL,
		wxLIST_STATE_SELECTED);
	return item;
}

void SyncMode::OnSyncNow(wxCommandEvent &event)
{
	DeviceSet::subset_type subset = GetSelectedDevices();
	if( subset.size() == 0 )
		return;	// nothing to do

	// make sure an app is not running
	if( m_cui.get() && m_cui->IsAppRunning() ) {
		wxMessageBox(_T("An application is currently running."),
			_T("Sync Error"), wxOK | wxICON_ERROR);
		return;
	}

	SyncStatusDlg dlg(m_parent, subset);
	dlg.ShowModal();
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
		wxMessageBox(_T("An application is already running."),
			_T("Run App Error"), wxOK | wxICON_ERROR);
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
}

void SyncMode::OnListSelChange(wxListEvent &event)
{
	UpdateButtons();
}

void SyncMode::OnConfigureDevice(wxListEvent &event)
{
	ConfigureDevice(event.GetIndex());
}

