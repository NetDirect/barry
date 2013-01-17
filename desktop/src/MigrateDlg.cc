///
/// \file	MigrateDlg.cc
///		Dialog for the "Migrate Device" main menu mode button...
///		going with a dialog instead of a mode class this time.
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "MigrateDlg.h"
#include "windowids.h"
#include "configui.h"
#include "barrydesktop.h"
#include <string>
#include <wx/statline.h>
#include <barry/barrybackup.h>
#include <iostream>
#include "wxi18n.h"

using namespace std;
using namespace OpenSync;

DEFINE_EVENT_TYPE(MET_THREAD_FINISHED)
DEFINE_EVENT_TYPE(MET_CHECK_DEST_PIN)
DEFINE_EVENT_TYPE(MET_SET_STATUS_MSG)
DEFINE_EVENT_TYPE(MET_PROMPT_PASSWORD)
DEFINE_EVENT_TYPE(MET_ERROR_MSG)

BEGIN_EVENT_TABLE(MigrateDlg, wxDialog)
	EVT_BUTTON	(Dialog_Migrate_MigrateNowButton,
				MigrateDlg::OnMigrateNow)
	EVT_BUTTON	(Dialog_Migrate_CancelButton,
				MigrateDlg::OnCancel)
	EVT_CLOSE	(MigrateDlg::OnCloseWindow)
	EVT_COMMAND	(wxID_ANY, MET_THREAD_FINISHED,
				MigrateDlg::OnThreadFinished)
	EVT_COMMAND	(wxID_ANY, MET_CHECK_DEST_PIN,
				MigrateDlg::OnCheckDestPin)
	EVT_COMMAND	(wxID_ANY, MET_SET_STATUS_MSG,
				MigrateDlg::OnSetStatusMsg)
	EVT_COMMAND	(wxID_ANY, MET_PROMPT_PASSWORD,
				MigrateDlg::OnPromptPassword)
	EVT_COMMAND	(wxID_ANY, MET_ERROR_MSG,
				MigrateDlg::OnErrorMsg)
END_EVENT_TABLE()


class EventDesktopConnector : public Barry::DesktopConnector
{
	MigrateDlg *m_dlg;

public:
	EventDesktopConnector(MigrateDlg *dlg, const char *password,
		const std::string &locale, const Barry::ProbeResult &result)
		: Barry::DesktopConnector(password, locale, result)
		, m_dlg(dlg)
	{
	}

	virtual bool PasswordPrompt(const Barry::BadPassword &bp,
					std::string &password_result);
};

bool EventDesktopConnector::PasswordPrompt(const Barry::BadPassword &bp,
					std::string &password_result)
{
	// ping the parent and wait for finish
	wxCommandEvent event(MET_PROMPT_PASSWORD, wxID_ANY);
	event.SetEventObject(m_dlg);
	event.SetInt(bp.remaining_tries());
	m_dlg->AddPendingEvent(event);
	m_dlg->WaitForEvent();

	password_result = m_dlg->GetPassword().utf8_str();

	// assume that a blank password means the user wishes to quit...
	// wxWidgets doesn't seem to handle this very well?
	return password_result.size() > 0;
}


//////////////////////////////////////////////////////////////////////////////
// MigrateDlg class

MigrateDlg::MigrateDlg(wxWindow *parent,
			const Barry::Probe::Results &results,
			int current_device_index)
	: wxDialog(parent, Dialog_GroupCfg, _W("Migrate Device"))
	, m_results(results)
	, m_current_device_index(current_device_index)
	, m_migrate_thread_created(false)
	, m_abort_flag(false)
	, m_thread_running(false)
	, m_source_device(0)
	, m_dest_device(0)
	, m_topsizer(0)
	, m_source_combo(0)
	, m_dest_combo(0)
	, m_write_mode_combo(0)
	, m_migrate_button(0)
	, m_wipe_check(0)
	, m_status(0)
	, m_progress(0)
{
	// setup the raw GUI
	CreateLayout();
}

void MigrateDlg::WaitForEvent()
{
	m_waiter.Wait();
}

void MigrateDlg::CreateLayout()
{
	m_topsizer = new wxBoxSizer(wxVERTICAL);

//	AddDescriptionSizer(m_topsizer);
	AddMainSizer(m_topsizer);
	AddStatusSizer(m_topsizer);

	SetSizer(m_topsizer);
	m_topsizer->SetSizeHints(this);
	m_topsizer->Layout();
}

void MigrateDlg::AddDescriptionSizer(wxSizer *sizer)
{
	sizer->Add(
		new wxStaticText(this, wxID_ANY,
			_W("Migrate device data from source device to target device.")),
		0, wxALIGN_CENTRE | wxALIGN_CENTRE_VERTICAL |
			wxTOP | wxLEFT | wxRIGHT, 10);
}

void MigrateDlg::AddMainSizer(wxSizer *sizer)
{
	// add 3 main sections together into one sizer
	wxSizer *main = new wxBoxSizer(wxHORIZONTAL);
	Main_AddSourceSizer(main);
	Main_AddButtonSizer(main);
	Main_AddDestSizer(main);

	// add main sizer to top level sizer
	sizer->Add(main, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
}

void MigrateDlg::AddStatusSizer(wxSizer *sizer)
{
	sizer->Add( new wxStaticLine(this),
		0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);

	sizer->Add(
		m_status = new wxStaticText(this, wxID_ANY, _W("Ready...")),
		0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	// Reduce font size for status text
	wxFont font = m_status->GetFont();
	font.SetPointSize(font.GetPointSize() - 1);
	m_status->SetFont( font );

	sizer->Add(
		m_progress = new wxGauge(this, wxID_ANY, 100),
		0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
}

void MigrateDlg::Main_AddSourceSizer(wxSizer *sizer)
{
	wxArrayString devices;
	for( Barry::Probe::Results::const_iterator i = m_results.begin();
				i != m_results.end(); ++i )
	{
		devices.Add(wxString(i->GetDisplayName().c_str(), wxConvUTF8));
	}

	wxSizer *source = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _W("Source device")),
		wxVERTICAL
		);
	source->Add(
		m_source_combo = new wxChoice(this, wxID_ANY,
			wxDefaultPosition, wxSize(225, -1), devices),
		0, wxALL, 5);

	if( m_current_device_index >= 0 )
		m_source_combo->SetSelection(m_current_device_index);

	sizer->Add(source, 0, wxEXPAND, 0);
}

void MigrateDlg::Main_AddButtonSizer(wxSizer *sizer)
{
	wxSizer *buttons = new wxBoxSizer(wxVERTICAL);
	buttons->Add( m_migrate_button = new wxButton(this,
		Dialog_Migrate_MigrateNowButton, _W("Migrate Now")),
		0, wxALIGN_CENTRE, 0);
	m_migrate_button->SetDefault();
	buttons->AddSpacer(10);
	buttons->Add( new wxButton(this, Dialog_Migrate_CancelButton,
			_W("Cancel")),
		0, wxALIGN_CENTRE, 0);

	sizer->Add(buttons, 1, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 10);
}

void MigrateDlg::Main_AddDestSizer(wxSizer *sizer)
{
	wxArrayString devices;
	devices.Add(_W("Prompt to plug in later..."));
	for( Barry::Probe::Results::const_iterator i = m_results.begin();
				i != m_results.end(); ++i )
	{
		devices.Add(wxString(i->GetDisplayName().c_str(), wxConvUTF8));
	}

	wxSizer *dest = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _W("Destination device")),
		wxVERTICAL
		);
	dest->Add(
		m_dest_combo = new wxChoice(this, wxID_ANY,
			wxDefaultPosition, wxSize(225, -1), devices),
		0, wxALL, 5);
	m_dest_combo->SetSelection(0);


	wxArrayString write_modes;
	// TRANSLATORS: these 4 strings are write-mode options in the
	// Migrate Device dialog
	write_modes.Add(_W("Erase all, then restore"));
	write_modes.Add(_W("Add new, and overwrite existing"));
	write_modes.Add(_W("Add only, don't overwrite existing"));
	write_modes.Add(_W("Add every record as a new entry (may cause duplicates)"));

	dest->Add( new wxStaticText(this, wxID_ANY, _W("Write Mode:")),
		0, wxTOP | wxLEFT | wxRIGHT, 5);
	dest->Add( m_write_mode_combo = new wxChoice(this, wxID_ANY,
			wxDefaultPosition, wxSize(225, -1), write_modes),
		0, wxALL, 5);
	m_write_mode_combo->SetSelection(0);

//	dest->Add( m_wipe_check = wxCheckBox(maybe a checkbox for "wipe device before restore"));

	sizer->Add(dest, 0, wxEXPAND, 0);
}

void MigrateDlg::EnableControls(bool enable)
{
	m_source_combo->Enable(enable);
	m_dest_combo->Enable(enable);
	m_write_mode_combo->Enable(enable);
	m_migrate_button->Enable(enable);
	//m_wipe_check->Enable(enable);
}

void MigrateDlg::DoSafeClose()
{
	// if migrate thread is running, try to close it down first...
	// do not exit the dialog until the thread is properly stopped!
	if( m_thread_running ) {
		m_abort_flag = true;

		m_status->SetLabel(_W("Waiting for thread to close..."));

		if( m_migrate_thread_created ) {
			void *junk;
			pthread_join(m_migrate_thread, &junk);
			m_migrate_thread_created = false;
		}
	}

	// all activity is stopped, so close dialog
	EndModal(wxID_CANCEL);
}

void MigrateDlg::SendEvent(int event_type)
{
	wxCommandEvent event(event_type, wxID_ANY);
	event.SetEventObject(this);
	AddPendingEvent(event);
}

void MigrateDlg::SendStatusEvent(const wxString &msg, int pos, int max)
{
	wxCommandEvent event(MET_SET_STATUS_MSG, wxID_ANY);
	event.SetEventObject(this);

	event.SetString(msg);

	int value = 0;
	if( pos == -1 )
		value |= 0xff00;
	else
		value |= (pos << 8);

	if( max == -1 )
		value |= 0xff;
	else
		value |= max & 0xff;
	event.SetInt(value);

	AddPendingEvent(event);
}

void MigrateDlg::SendErrorMsgEvent(const wxString &msg)
{
	wxCommandEvent event(MET_ERROR_MSG, wxID_ANY);
	event.SetEventObject(this);
	event.SetString(msg);
	AddPendingEvent(event);
}

void MigrateDlg::OnMigrateNow(wxCommandEvent &event)
{
	// gather info from dialog
	int source_index = m_source_combo->GetSelection();
	int dest_index = m_dest_combo->GetSelection();
	int write_mode_index = m_write_mode_combo->GetSelection();

	// validate options
	if( source_index == wxNOT_FOUND || dest_index == wxNOT_FOUND ||
		write_mode_index == wxNOT_FOUND )
	{
		wxMessageBox(_W("Please select a source and destination device, as well as the write mode."),
			_W("Migration Options Needed"), wxOK | wxICON_ERROR, this);
		return;
	}

	// do not migrate from one PIN to the same PIN
	if( source_index == (dest_index - 1) ) {
		wxMessageBox(_W("Cannot migrate from and to the same PIN."),
			_W("Migration Options Error"), wxOK | wxICON_ERROR, this);
		return;
	}

	// set the migration arguments
	m_source_device = &m_results[source_index];
	if( dest_index > 0 ) {
		m_dest_device = &m_results[dest_index-1];
	}
	else {
		m_dest_device = 0; // an invalid dest causes a prompt
	}
	switch( write_mode_index )
	{
	case 0:
		m_write_mode = Barry::DeviceParser::ERASE_ALL_WRITE_ALL;
		break;
	case 1:
		m_write_mode = Barry::DeviceParser::INDIVIDUAL_OVERWRITE;
		break;
	case 2:
		m_write_mode = Barry::DeviceParser::ADD_BUT_NO_OVERWRITE;
		break;
	case 3:
		m_write_mode = Barry::DeviceParser::ADD_WITH_NEW_ID;
		break;
	default:
		wxMessageBox(_W("Invalid write mode. This should never happen. Contact the developers."),
			_W("Internal Logic Error"), wxOK | wxICON_ERROR, this);
		return;
	}

	// disable all buttons and controls except cancel
	EnableControls(false);

	// turn off the stop flag
	m_abort_flag = false;
	m_thread_running = false;

	// fire up migrate thread, and let the thread close the dialog when
	// done  (can we EndModal() from a thread?)
	int ret = pthread_create(&m_migrate_thread, NULL,
			&MigrateDlg::MigrateThread, this);
	if( ret != 0 ) {
		// go back to normal
		EnableControls(true);
		return;
	}
	else {
		m_migrate_thread_created = true;
	}

	// thread started... let it finish
}

void MigrateDlg::OnCancel(wxCommandEvent &event)
{
	DoSafeClose();
}

void MigrateDlg::OnCloseWindow(wxCloseEvent &event)
{
	DoSafeClose();
}

void MigrateDlg::OnThreadFinished(wxCommandEvent &event)
{
	if( m_migrate_thread_created ) {
		m_status->SetLabel(_W("Waiting for thread..."));
		void *junk;
		pthread_join(m_migrate_thread, &junk);
		m_migrate_thread_created = false;
	}

	if( m_abort_flag ) {
		// user cancelled in some way, restore GUI
		EnableControls(true);
		m_status->SetLabel(_W("Cancelled by user..."));
	}
	else {
		// if we were not aborted, then this is success, and we
		// can close the original dialog
		EndModal(wxID_CANCEL);
	}
}

void MigrateDlg::OnCheckDestPin(wxCommandEvent &event)
{
	ScopeSignaler done(m_waiter);

	if( m_dest_device && m_dest_device->m_pin.Valid() )
		return; // nothing to do

	// no destination pin was available, so user may need to plugin
	// the new device right now, before continuing
	int response = wxMessageBox(_W("Please plug in the target device now."),
		_W("Ready for Writing"), wxOK | wxCANCEL | wxICON_INFORMATION,
		this);
	if( response != wxOK ) {
		// user cancelled
		m_abort_flag = true;
		return;
	}

	{
		wxBusyCursor wait;
		m_status->SetLabel(_W("Scanning USB for devices..."));

		// pause for 2 seconds to let any new devices settle down
		wxGetApp().Yield();
		wxSleep(2);

		// rescan the USB and try to find the new device
		Barry::Probe probe;
		m_new_results = probe.GetResults();
	}

	// now prompt the user... create a list of PIN display names
	wxArrayString devices;
	for( Barry::Probe::Results::const_iterator i = m_new_results.begin();
				i != m_new_results.end(); ++i )
	{
		devices.Add(wxString(i->GetDisplayName().c_str(), wxConvUTF8));
	}

	m_status->SetLabel(_W("User input..."));

	do {

		// prompt the user with this list
		int choice = wxGetSingleChoiceIndex(_W("Please select the target device to write to:"),
			_W("Destination PIN"), devices, this);
		if( choice == -1 ) {
			// user cancelled
			m_abort_flag = true;
			return;
		}

		// found the new PIN to use!
		m_dest_device = &m_new_results[choice];

		// check if user needs to choose again
		if( m_dest_device->m_pin == m_source_device->m_pin ) {
			wxMessageBox(_W("Cannot use the same device PIN as migration destination."),
				_W("Invalid Device Selection"),
				wxOK | wxICON_ERROR, this);
		}

	} while( m_dest_device->m_pin == m_source_device->m_pin );
}

void MigrateDlg::OnSetStatusMsg(wxCommandEvent &event)
{
	if( event.GetString().size() ) {
		m_status->SetLabel(event.GetString());
	}

	if( (unsigned int)event.GetInt() != 0xffff ) {
		unsigned int value = (unsigned int) event.GetInt();
		unsigned int pos = (value & 0xff00) >> 8;
		unsigned int max = (value & 0xff);

		if( pos != 0xff ) {
			m_progress->SetValue(pos);
		}

		if( max != 0xff ) {
			m_progress->SetRange(max);
		}
	}
}

void MigrateDlg::OnPromptPassword(wxCommandEvent &event)
{
	ScopeSignaler done(m_waiter);

	// create prompt based on exception data
	wxString prompt = wxString::Format(
		_W("Please enter device password: (%d tries remaining)"),
		event.GetInt());

	// ask user for device password
	m_password = wxGetPasswordFromUser(prompt,
		_W("Device Password"), _T(""), this);
}

void MigrateDlg::OnErrorMsg(wxCommandEvent &event)
{
	ScopeSignaler done(m_waiter);
	wxMessageBox(event.GetString(), _W("Migration Error"),
		wxOK | wxICON_ERROR, this);
}

void* MigrateDlg::MigrateThread(void *arg)
{
	MigrateDlg *us = (MigrateDlg*) arg;

	// we are running!
	us->m_thread_running = true;

	try {
		// backup source PIN
		us->BackupSource();

		// make sure we have a destination PIN
		if( !us->m_abort_flag )
			us->CheckDestPin();

		// restore to dest PIN
		if( !us->m_abort_flag )
			us->RestoreToDest();
	}
	catch( std::exception &e ) {
		us->SendErrorMsgEvent(wxString(e.what(), wxConvUTF8));
		us->m_waiter.Wait();
	}

	// invalidate the device selection pointers, since
	// m_new_results may not always exist
	us->m_source_device = us->m_dest_device = 0;

	// we are done!
	us->m_thread_running = false;

	// send event to let main GUI thread we're finished
	us->SendEvent(MET_THREAD_FINISHED);

	return 0;
}

class PeekParser : public Barry::Parser
{
	std::string m_dbname;

public:
	virtual void ParseRecord(const Barry::DBData &data,
				const Barry::IConverter *ic)
	{
		m_dbname = data.GetDBName();
	}

	const std::string& GetDBName() const { return m_dbname; }
};


// This is called from the thread
void MigrateDlg::BackupSource()
{
	// connect to the source device
	SendStatusEvent(_W("Connecting..."));
	EventDesktopConnector connect(this, "", "utf-8", *m_source_device);
	if( !connect.Reconnect(2) ) {
		// user cancelled
		m_abort_flag = true;
		return;
	}

	// fetch DBDB, for list of databases to backup... back them all up
	// remember to save this DBDB into the class, so it is available
	// for the restore stage
	m_source_dbdb = connect.GetDesktop().GetDBDB();
	unsigned int total = m_source_dbdb.GetTotalRecordCount();

	// create DeviceBuilder for fetching all
	Barry::DeviceBuilder builder(connect.GetDesktop());
	builder.Add(m_source_dbdb);

	// calculate the default backup path location, based on user name
	// (see backup GUI for code?)
	Barry::ConfigFile cfg(m_source_device->m_pin);
	Barry::ConfigFile::CheckPath(cfg.GetPath());
	m_backup_tarfile = cfg.GetPath() + "/" +
		Barry::MakeBackupFilename(m_source_device->m_pin, "migrate");

	// and create tarball output parser
	Barry::Backup parser(m_backup_tarfile);

	// create the pipe
	Barry::Pipe pipe(builder);

	PeekParser peeker;

	// create tee parser, so we can see what's being written
	Barry::TeeParser tee;
	tee.Add(peeker);
	tee.Add(parser);

	// setup status bar
	unsigned int count = 0;
	SendStatusEvent(_T(""), 0, 100);

	// cycle through all databases
	while( !builder.EndOfFile() ) {
		if( !pipe.PumpEntry(tee) )
			continue;

		count++;

		// ok, first entry has been pumped, so we can use
		// peeker to create the status message, and only once
		ostringstream oss;
		oss << _C("Backing up database: ")
		    << peeker.GetDBName()
		    << "...";

		// calculate 1 to 100 percentage, based on number of
		// databases being backed up, and update status bar too
		int percent = count / (float)total * 100.0;

		// send status event once
		SendStatusEvent(wxString(oss.str().c_str(),wxConvUTF8), percent);

		// backup the rest of the database, but don't update status
		// for each record
		int block = 0;
		while( pipe.PumpEntry(tee) ) {
			count++;
			block++;
			if( block >= 25 ) {
				int percent = count / (float)total * 100.0;
				SendStatusEvent(_T(""), percent);
				block = 0;
			}

			// on each record (pump cycle?), as often as possible,
			// check the m_abort_flag, and abort if necessary,
			// updating the status message
			if( m_abort_flag ) {
				SendStatusEvent(_W("Backup aborted by user..."));
				return;
			}
		}
	}

	// close all files, etc.
	parser.Close();
}

// This is called from the thread
void MigrateDlg::CheckDestPin()
{
	SendEvent(MET_CHECK_DEST_PIN);
	m_waiter.Wait();
}

// This is called from the thread
void MigrateDlg::RestoreToDest()
{
	// connect to the dest device
	SendStatusEvent(_W("Connecting to target..."));
	EventDesktopConnector connect(this, "", "utf-8", *m_dest_device);
	if( !connect.Reconnect(2) ) {
		// user cancelled
		m_abort_flag = true;
		return;
	}

	// fetch DBDB of dest device, for list of databases we can restore
	// to... compare with the backup DBDB to create a list of similarly
	// named databases which we can restore....
	Barry::DatabaseDatabase dest_dbdb = connect.GetDesktop().GetDBDB();

	// create the restore builder object, to read from tarball
	Barry::Restore builder(m_backup_tarfile);

	// add all database names from the _dest_ DBDB to the Restore
	// filter... this way, only databases that exist on the new
	// device will get restored
	builder.Add(dest_dbdb);
	unsigned int total = builder.GetRecordTotal();// counts filtered records

	// create device parser, to write to device
	Barry::DeviceParser parser(connect.GetDesktop(), m_write_mode);

	// create the pipe
	Barry::Pipe pipe(builder);

	// setup status bar
	unsigned int count = 0;
	SendStatusEvent(_T(""), 0, 100);

	// cycle through all databases
	Barry::DBData meta;		// record meta data of next tar record
	while( !builder.EndOfFile() ) try {
		if( !builder.GetNextMeta(meta) )
			continue;

		// ok, first entry has been pumped, so we can use
		// peeker to create the status message, and only once
		ostringstream oss;
		oss << _C("Writing database: ")
		    << meta.GetDBName()
		    << "...";

		// for debugging purposes in the field, display the names
		// of the databases we restore
		cerr << oss.str() << endl;

		// calculate 1 to 100 percentage, based on number of
		// databases being backed up, and update status bar too
		int percent = count / (float)total * 100.0;

		// send status event once
		SendStatusEvent(wxString(oss.str().c_str(),wxConvUTF8), percent);

		// restore  the rest of the database, but don't update status
		// for each record
		int block = 0;
		while( pipe.PumpEntry(parser) ) {
			count++;
			block++;
			if( block >= 25 ) {
				int percent = count / (float)total * 100.0;
				SendStatusEvent(_T(""), percent);
				block = 0;
			}

			// on each record (pump cycle?), as often as possible,
			// check the m_abort_flag, and abort if necessary,
			// updating the status message
			if( m_abort_flag ) {
				SendStatusEvent(_W("Restore aborted by user..."));
				return;
			}
		}
	}
	catch( Barry::ReturnCodeError &e ) {
		cerr << _C("Unable to clear or write to database: ")
		     << meta.GetDBName()
		     << ": " << e.what() << endl;
		cerr << _C("Continuing to process remaining records...") << endl;

		// skip the problematic database, and keep on trying
		builder.SkipCurrentDB();
	}
}

