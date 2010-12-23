///
/// \file	BackupWindow.cc
///		GUI window class
///

/*
    Copyright (C) 2007-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "BackupWindow.h"
#include "PasswordDlg.h"
#include "PromptDlg.h"
#include "ConfigDlg.h"
#include "util.h"
#include "i18n.h"
#include <gtkmm/aboutdialog.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unistd.h>

BackupWindow::BackupWindow(BaseObjectType *cobject,
			   const Glib::RefPtr<Gnome::Glade::Xml> &xml)
	: Gtk::Window(cobject)
	, m_xml(xml)
	, m_pStatusbar(0)
	, m_pBackupButton(0)
	, m_pRestoreButton(0)
	, m_pDisconnectButton(0)
	, m_pReloadButton(0)
	, m_pDeviceLabel(0)
	, m_pDeviceList(0)
	, m_device_count(0)
	, m_pActive(0)
	, m_scanned(false)
	, m_last_status_id(0)
{
	m_signal_update.connect(
		sigc::mem_fun(*this, &BackupWindow::treeview_update));

	// setup menu signals
	Gtk::MenuItem *pItem = 0;
	m_xml->get_widget("menu_file_quit", pItem);
	pItem->signal_activate().connect(
		sigc::mem_fun(*this, &BackupWindow::on_file_quit));

	m_xml->get_widget("menu_help_about", pItem);
	pItem->signal_activate().connect(
		sigc::mem_fun(*this, &BackupWindow::on_help_about));

	// get various widget pointers we will use later
	m_xml->get_widget("DeviceLabel", m_pDeviceLabel);
	m_xml->get_widget("DeviceList", m_pDeviceList);
	m_xml->get_widget("BackupButton", m_pBackupButton);
	m_xml->get_widget("RestoreButton", m_pRestoreButton);
	m_xml->get_widget("ConfigButton", m_pConfigButton);
	m_xml->get_widget("DisconnectButton", m_pDisconnectButton);
	m_xml->get_widget("DisconnectAllButton", m_pDisconnectAllButton);
	m_xml->get_widget("ReloadButton", m_pReloadButton);
	m_xml->get_widget("Statusbar", m_pStatusbar);

	// set up device list
	m_pListStore = Gtk::ListStore::create(m_columns);
	m_pDeviceList->set_model(m_pListStore);

	m_pDeviceList->append_column(_("PIN"), m_columns.m_pin);
	m_pDeviceList->append_column(_("Name"), m_columns.m_name);
	m_pDeviceList->append_column(_("Status"), m_columns.m_status);
	Gtk::CellRendererProgress* cell = new Gtk::CellRendererProgress;
	m_pDeviceList->append_column(_("Progress"), *cell);
	Gtk::TreeViewColumn* pColumn = m_pDeviceList->get_column(3);
	pColumn->add_attribute(cell->property_value(), m_columns.m_percentage);

	m_pDeviceList->get_column(0)->set_min_width(60);
	m_pDeviceList->get_column(1)->set_min_width(100);
	m_pDeviceList->get_column(2)->set_min_width(75);

	for( unsigned int i = 0; i < 4; ++i )
		m_pDeviceList->get_column(i)->set_resizable();

	// set up device list selection
	m_pDeviceSelection = m_pDeviceList->get_selection();

	// setup widget signals
	m_pBackupButton->signal_clicked().connect(
		sigc::mem_fun(*this, &BackupWindow::on_backup));
	m_pRestoreButton->signal_clicked().connect(
		sigc::mem_fun(*this, &BackupWindow::on_restore));
	m_pConfigButton->signal_clicked().connect(
		sigc::mem_fun(*this, &BackupWindow::on_config));
	m_pDisconnectButton->signal_clicked().connect(
		sigc::mem_fun(*this, &BackupWindow::on_disconnect));
	m_pDisconnectAllButton->signal_clicked().connect(
		sigc::mem_fun(*this, &BackupWindow::on_disconnect_all));
	m_pReloadButton->signal_clicked().connect(
		sigc::mem_fun(*this, &BackupWindow::on_reload));
	m_pDeviceSelection->signal_changed().connect(
		sigc::mem_fun(*this, &BackupWindow::on_device_change));

	// setup startup device scan
	Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &BackupWindow::on_startup), 500);

	// workaround: normally this should say "Ready" but since
	// the initial Scan() happens right away, and the statusbar
	// doesn't seem to update the screen until the handler is
	// finished, we update the status bar here instead
	StatusbarSet(_("Scanning for devices..."));

	// do this last so that any exceptions in the constructor
	// won't cause a connected signal handler to a non-object
	// (i.e. ~BackupWindow() won't get called if constructor throws)
	m_signal_handler_connection = Glib::add_exception_handler(
		sigc::mem_fun(*this, &BackupWindow::signal_exception_handler) );
}

BackupWindow::~BackupWindow()
{
	// disconnect the signal, as we're going out of business
	m_signal_handler_connection.disconnect();
}

void BackupWindow::Scan()
{
	StatusbarSet(_("Scanning for devices..."));

	m_pListStore->clear();
	m_threads.clear();

	m_bus.Probe();
	m_device_count = m_bus.ProbeCount();

	if( m_device_count == 0 )
		m_pDeviceLabel->set_label(_("No devices."));
	else if( m_device_count == 1 )
		m_pDeviceLabel->set_label(_("1 device:"));
	else
	{
		std::ostringstream oss;
		oss << m_device_count << _(" devices:");
		m_pDeviceLabel->set_label(oss.str());
	}

	m_threads.resize(m_device_count);
	for( unsigned int id = 0; id < m_device_count; ++id ) {
		Device dev = m_bus.Get(id);
		Gtk::TreeModel::iterator row = m_pListStore->append();
		(*row)[m_columns.m_id] = id;

		m_threads[id].reset(new Thread(dev, &m_signal_update));
	}

	// all devices loaded
	m_scanned = true;

	StatusbarSet(_("All devices loaded."));

	// if one or more device plugged in,
	// activate the first one
	Gtk::TreeModel::iterator iter = m_pListStore->children().begin();
	if( iter )
		m_pDeviceSelection->select(iter);
}

bool BackupWindow::Connect(Thread *thread)
{
	if( thread->Connected() )
		return true;

	StatusbarSet(_("Connecting to Device..."));
	static int tries(0);

	CheckDeviceName(thread);

	if( !thread->Connect() ) {
		if( thread->PasswordRequired() ) {
			bool connected = false;
			while( !connected && !thread->PasswordOutOfTries() ) {
				PasswordDlg dlg(thread->PasswordRemainingTries());
				if( dlg.run() == Gtk::RESPONSE_OK )
					connected = thread->Connect(dlg.GetPassword());
				else { // user cancelled
					thread->Reset();
					StatusbarSet(_("Connection cancelled."));
					return false;
				}
			}
			if( thread->PasswordOutOfTries() )
			{
				Gtk::MessageDialog msg(thread->BadPasswordError());
				msg.run();
				StatusbarSet(_("Cannot connect to ") + thread->GetFullname() + ".");
				return false;
			}
		}
		else if( thread->BadSize() ) {
			++tries;
			if( tries < 3 ) {
				std::cerr << thread->BadSizeError() << std::endl;
				thread->Reset();
				sleep(2);
				return Connect(thread);
			}
			else {
				Gtk::MessageDialog msg(thread->BadSizeError());
				msg.run();
				StatusbarSet(_("Cannot connect to ") + thread->GetFullname() + ".");
				return false;
			}
		}
		else {
			Gtk::MessageDialog msg(thread->LastInterfaceError());
			msg.run();
			StatusbarSet(_("Cannot connect to ") + thread->GetFullname() + ".");
			return false;
		}
	}
	tries = 0;
	StatusbarSet(_("Connected to ") + thread->GetFullname() + ".");
	return true;
}

void BackupWindow::Disconnect(Thread *thread)
{
	if( thread->Working() ) {
		Gtk::MessageDialog dialog(*this, thread->GetFullname() + _(" is working, "
			"disconnecting from it may cause data corruption, are you sure to proceed?"),
			false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
		if( dialog.run() == Gtk::RESPONSE_CANCEL )
			return;
	}

	if( thread->Connected() ) {
		thread->Disconnect();
		StatusbarSet(_("Disconnected from ") + thread->GetFullname() + ".");
	}
	else {
		StatusbarSet(_("Not connected."));
	}
}

void BackupWindow::CheckDeviceName(Thread *thread)
{
	if( !thread->HasDeviceName() ) {
		PromptDlg dlg;
		dlg.SetPrompt(_("Unnamed device found (PIN: ") + thread->GetPIN().Str() + ").\r\n " + _("Please enter a name for it:"));
		if( dlg.run() == Gtk::RESPONSE_OK ) {
			thread->SetDeviceName(dlg.GetAnswer());
		}
		else {
			thread->SetDeviceName(" ");
		}
		if( !thread->Save() ) {
			Gtk::MessageDialog msg(_("Error saving config: ") +
				thread->LastConfigError());
			msg.run();
		}
	}
}

Thread *BackupWindow::GetActive()
{
	Gtk::TreeModel::iterator row = m_pDeviceSelection->get_selected();
	if( row ) {
		unsigned int id = (*row)[m_columns.m_id];
		return m_threads[id].get();
	}
	else {
		return 0;
	}
}

void BackupWindow::StatusbarSet(const Glib::ustring& text)
{
	guint remove_id = m_last_status_id;
	if( m_pStatusbar ) {
		m_last_status_id = m_pStatusbar->push(text);
		if( remove_id )
			m_pStatusbar->remove_message(remove_id);
	}
}

/// Returns true if ok to proceed (either nothing is currently 'working',
/// or the user confirmed to do it anyway)
bool BackupWindow::CheckWorking()
{
	bool working(false);
	for( unsigned int i = 0; i < m_device_count; ++i) {
		if( m_threads[i]->Working() ) {
			working = true;
			break;
		}
	}

	if( working ) {
		Gtk::MessageDialog dialog(*this, _("One or more devices are working, "
			"disconnecting now may cause data corruption. "
			"Proceed anyway?"),
			false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
		if( dialog.run() != Gtk::RESPONSE_OK )
			return false;
	}

	return true;
}

void BackupWindow::signal_exception_handler()
{
	try {
		throw;
	}
	catch( Glib::Exception &e ) {
		// This usually just means a missing .glade file,
		// so we try to carry on.
		std::cerr << "Glib::Exception caught in main: " << std::endl;
		std::cerr << e.what() << std::endl;
		Gtk::MessageDialog msg(e.what());
		msg.run();
	}
	catch( ... ) {
		// anything else, terminate window and pass on to next handler
		// (which should be in main.cc)
		hide();
		throw;
	}
}


//////////////////////////////////////////////////////////////////////////////
// signal handlers

void BackupWindow::treeview_update()
{
	for( Gtk::TreeModel::iterator i = m_pListStore->children().begin();
		i != m_pListStore->children().end(); ++i ) {
		unsigned int id = (*i)[m_columns.m_id];
		Thread *thread = m_threads[id].get();
		(*i)[m_columns.m_pin] = thread->GetPIN().Str();
		(*i)[m_columns.m_name] = thread->GetDeviceName();
		(*i)[m_columns.m_status] = thread->Status();
		unsigned int finished(thread->GetRecordFinished()), total(thread->GetRecordTotal());
		unsigned int percentage(0);
		if( total == 0 || finished == total )
			percentage = 100;
		else {
			percentage = 100 * finished / total;
			if( percentage == 100 ) // never say 100% unless finished
				percentage = 99;
		}
		(*i)[m_columns.m_percentage] = percentage;
		if( thread->CheckFinishedMarker() ) {
			std::string op;

			if( thread->GetThreadState() & THREAD_STATE_BACKUP )
				op = _("Backup on");
			else if( thread->GetThreadState() & THREAD_STATE_RESTORE )
				op = _("Restore on");
			else
				op = _("Operation on");

			StatusbarSet(op + thread->GetFullname() + _(" finished!"));
			if( (thread->GetThreadState() & THREAD_STATE_BACKUP) &&
			     finished != total )
			{
				// in some cases, not all records are backed
				// up, possibly due to international chars
				// in the Blackberry data which somehow
				// forces the device to use a different
				// low level protocol... here we need to
				// warn the user that not all data was
				// included in the backup
				std::ostringstream oss;
				oss << _("Warning\n\nNot all records were processed on device: ") << thread->GetFullname()
					<< _("\n\nOnly ") << finished << _(" of ") << total
					<< _(" records were backed up.\n\nIt is suspected that due to international characters in these records, the BlackBerry uses a different low-level protocol, which Barry Backup does not yet support. Please contact the developers at http://netdirect.ca/barry if you want to assist in debugging this issue.");
				Gtk::MessageDialog msg(oss.str());
				msg.run();
			}
		}
	}
}

void BackupWindow::on_backup()
{
	Thread *thread = GetActive();

	if( !thread || !Connect(thread) )
		return;

	// already working?
	if( thread->Working() ) {
		Gtk::MessageDialog msg(_("Thread already in progress."));
		msg.run();
		return;
	}

	// make sure our target directory exists
	if( !Barry::ConfigFile::CheckPath(thread->GetPath()) ) {
		Gtk::MessageDialog msg(_("Could not create directory: ") + thread->GetPath());
		msg.run();
		return;
	}

	// anything to do?
	if( thread->GetBackupList().size() == 0 ) {
		Gtk::MessageDialog msg(_("No databases selected in configuration."));
		msg.run();
		return;
	}

	// prompt for a backup label, if so configured
	std::string backupLabel;
	if( thread->PromptBackupLabel() ) {
		PromptDlg dlg;
		dlg.SetPrompt(_("Please enter a label for this backup (blank is ok):"));
		if( dlg.run() == Gtk::RESPONSE_OK ) {
			backupLabel = dlg.GetAnswer();
		}
		else {
			// user cancelled
			return;
		}
	}

	// start the thread
	if( !thread->Backup(backupLabel) ) {
		Gtk::MessageDialog msg(_("Error starting backup thread: ") +
			thread->LastInterfaceError());
		msg.run();
	}
	else {
		StatusbarSet(_("Backup of ") + thread->GetFullname() + _(" in progress..."));
	}
}

bool BackupWindow::PromptForRestoreTarball(std::string &restoreFilename,
					   const std::string &start_path)
{
	char buffer[PATH_MAX];
	char *buf = getcwd(buffer, PATH_MAX);

	// start at the base path given... if it fails, just open
	// the dialog where we are
	chdir(start_path.c_str());

	Gtk::FileChooserDialog dlg(*this, _("Select backup to restore from"));
	dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	int result = dlg.run();

	if( buf )
		chdir(buf);

	if( result != Gtk::RESPONSE_OK )
		return false;

	restoreFilename = dlg.get_filename();
	return true;
}

void BackupWindow::on_restore()
{
	Thread *thread = GetActive();

	if( !thread || !Connect(thread) )
		return;

	// already working?
	if( thread->Working() ) {
		Gtk::MessageDialog msg(_("Thread already in progress."));
		msg.run();
		return;
	}

	std::string restoreFilename;
	if( !PromptForRestoreTarball(restoreFilename, thread->GetPath()) )
		return;	// nothing to do

	// start the thread
	if( !thread->Restore(restoreFilename) ) {
		Gtk::MessageDialog msg(_("Error starting restore thread: ") +
			thread->LastInterfaceError());
		msg.run();
	}
	else {
		StatusbarSet(_("Restore of ") + thread->GetFullname() + _(" in progress..."));
	}
}

void BackupWindow::on_disconnect()
{
	Thread *thread = GetActive();
	if( thread )
		Disconnect(thread);
}

void BackupWindow::on_disconnect_all()
{
	for( unsigned int i = 0; i < m_device_count; ++i )
		Disconnect(m_threads[i].get());
}

void BackupWindow::on_device_change()
{
	Thread *thread = GetActive();
	if( m_pActive )
		m_pActive->UnsetActive();
	m_pActive = thread;
	if( thread && Connect(thread) )
		thread->SetActive();
}

void BackupWindow::on_config()
{
	Thread *thread = GetActive();

	if( !thread || !Connect(thread) )
		return;

	thread->LoadConfig();

	ConfigDlg dlg(thread->GetDBDB(), *thread);
	if( dlg.run() == Gtk::RESPONSE_OK ) {
		thread->SetBackupList(dlg.GetBackupList());
		thread->SetRestoreList(dlg.GetRestoreList());
		thread->SetDeviceName(dlg.GetDeviceName());
		thread->SetBackupPath(dlg.GetBackupPath());
		thread->SetPromptBackupLabel(dlg.GetPromptBackupLabel());
		if( !thread->Save() )
			StatusbarSet(_("Error saving config: ") +
				thread->LastConfigError());
		else
			StatusbarSet(_("Config saved successfully."));
	}
	thread->LoadConfig();
}

void BackupWindow::on_reload()
{
	if( CheckWorking() ) {
		Scan();
	}
}

void BackupWindow::on_file_quit()
{
	if( CheckWorking() ) {
		hide();
	}
}

void BackupWindow::on_help_about()
{
	Gtk::AboutDialog dlg;
	dlg.set_copyright("Copyright (C) 2007-2010, Net Direct Inc.");
	dlg.set_license(
"    This program is free software; you can redistribute it and/or modify\n"
"    it under the terms of the GNU General Public License as published by\n"
"    the Free Software Foundation; either version 2 of the License, or\n"
"    (at your option) any later version.\n"
"\n"
"    This program is distributed in the hope that it will be useful,\n"
"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"\n"
"    See the GNU General Public License in the COPYING file at the\n"
"    root directory of this project for more details.\n");

	std::vector<std::string> authors;
	authors.push_back("Chris Frey <cdfrey@foursquare.net>");
	authors.push_back(_("and Barry contributors.  See AUTHORS file"));
	authors.push_back(_("for detailed contribution information."));

	dlg.set_authors(authors);

	int major, minor;
	const char *BarryVersion = Barry::Version(major, minor);
	dlg.set_name("Barry Backup");
	dlg.set_version("0.17");
	dlg.set_comments(std::string(_("Using library: ")) + BarryVersion);
	dlg.set_website("http://netdirect.ca/barry");
	dlg.run();
}

bool BackupWindow::on_startup()
{
	Scan();
	return false;
}

bool BackupWindow::on_delete_event(GdkEventAny *)
{
	if( CheckWorking() )
		return false;	// allow closing of window via window manager
	else
		return true;	// stop the close
}

