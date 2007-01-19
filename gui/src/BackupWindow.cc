///
/// \file	BackupWindow.cc
///		GUI window class
///

/*
    Copyright (C) 2007, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "DeviceSelectDlg.h"
#include "ConfigDlg.h"
#include <iostream>
#include <iomanip>
#include <sstream>

BackupWindow::BackupWindow(BaseObjectType *cobject,
			   const Glib::RefPtr<Gnome::Glade::Xml> &xml)
	: Gtk::Window(cobject)
	, m_xml(xml)
	, m_recordTotal(0)
	, m_finishedRecords(0)
	, m_pProgressBar(0)
	, m_pStatusBar(0)
	, m_pBackupButton(0)
	, m_pRestoreButton(0)
	, m_scanned(false)
	, m_working(false)
{
	// setup menu signals
	Gtk::MenuItem *pItem = 0;
	m_xml->get_widget("menu_file_quit", pItem);
	pItem->signal_activate().connect(
		sigc::mem_fun(*this, &BackupWindow::on_file_quit));

	m_xml->get_widget("menu_edit_config", pItem);
	pItem->signal_activate().connect(
		sigc::mem_fun(*this, &BackupWindow::on_edit_config));

	m_xml->get_widget("menu_help_about", pItem);
	pItem->signal_activate().connect(
		sigc::mem_fun(*this, &BackupWindow::on_help_about));

	// get various widget pointers we will use later
	m_xml->get_widget("BackupButton", m_pBackupButton);
	m_xml->get_widget("RestoreButton", m_pRestoreButton);
	m_xml->get_widget("progressbar1", m_pProgressBar);
	m_xml->get_widget("statusbar1", m_pStatusBar);
	m_xml->get_widget("entry1", m_pPINEntry);
	m_xml->get_widget("entry2", m_pDatabaseEntry);

	// setup widget signals
	m_pBackupButton->signal_clicked().connect(
		sigc::mem_fun(*this, &BackupWindow::on_backup));
	m_pRestoreButton->signal_clicked().connect(
		sigc::mem_fun(*this, &BackupWindow::on_restore));

	// setup thread dispatcher signals
	m_signal_progress.connect(
		sigc::mem_fun(*this, &BackupWindow::on_thread_progress));
	m_signal_done.connect(sigc::mem_fun(*this, &BackupWindow::on_thread_done));

	// setup startup device scan
	Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &BackupWindow::on_startup), 500);

	m_pStatusBar->push("Ready");
	m_pProgressBar->set_fraction(0.00);
}

BackupWindow::~BackupWindow()
{
}

void BackupWindow::ScanAndConnect()
{
	m_pStatusBar->push("Scanning for devices...");
	m_pStatusBar->show_now();

	Barry::Probe probe;
	uint32_t pin = 0;
	int nSelection = -1;

	if( probe.GetCount() > 1 ) {
		DeviceSelectDlg dlg(probe);
		if( dlg.run() == Gtk::RESPONSE_OK ) {
			pin = dlg.GetPIN();
			nSelection = probe.FindActive(pin);
		}
		else {
			// no selection, exit
			hide();
			return;
		}
	}
	else if( probe.GetCount() == 1 ) {
		// default to first
		pin = probe.Get(0).m_pin;
		nSelection = 0;
	}
	else {
		Gtk::MessageDialog msg("No BlackBerry devices found.");
		msg.run();
		hide();
		return;
	}

	if( nSelection == -1 ) {
		Gtk::MessageDialog msg("Internal error: unable to find pin.");
		msg.run();
		hide();
		return;
	}

	if( !m_dev.Connect(probe.Get(nSelection)) ) {
		Gtk::MessageDialog msg(m_dev.get_last_error());
		msg.run();
		hide();
		return;
	}

	std::ostringstream oss;
	oss << std::hex << pin;
	m_pPINEntry->set_text(oss.str());

	// open configuration now that we know which device we're talking to
	m_pConfig.reset( new ConfigFile(oss.str(), m_dev.GetDBDB()) );

	m_pStatusBar->pop();
}

void BackupWindow::SetWorkingMode()
{
	m_working = true;
	m_pBackupButton->set_sensitive(false);
	m_pRestoreButton->set_sensitive(false);
}

void BackupWindow::ClearWorkingMode()
{
	m_working = false;
	m_pBackupButton->set_sensitive(true);
	m_pRestoreButton->set_sensitive(true);
	m_pProgressBar->set_fraction(0.00);

	std::ostringstream oss;
	oss << m_recordTotal << " total records backed up.";
	m_pDatabaseEntry->set_text(oss.str());
}

void BackupWindow::UpdateProgress()
{
	double done = (double)m_finishedRecords / m_recordTotal;
	// never say 100% unless really done
	if( done >= 1.0 && m_finishedRecords < m_recordTotal ) {
		done = 0.99;
	}
	m_pProgressBar->set_fraction(done);

	m_pDatabaseEntry->set_text(m_dev.GetThreadDBName());
}


//////////////////////////////////////////////////////////////////////////////
// signal handlers

void BackupWindow::on_backup()
{
	// already working?
	if( m_working ) {
		Gtk::MessageDialog msg("Thread already in progress.");
		msg.run();
		return;
	}

	// prepare for the progress bar
	m_recordTotal = m_dev.GetDeviceRecordTotal(m_pConfig->GetBackupList());
	m_finishedRecords = 0;

	// start the thread
	m_working = m_dev.StartBackup(
		DeviceInterface::AppComm(&m_signal_progress, &m_signal_done),
		m_pConfig->GetBackupList(), m_pConfig->GetPath(),
		m_pConfig->GetPIN());
	if( !m_working ) {
		Gtk::MessageDialog msg("Error starting backup thread: " +
			m_dev.get_last_error());
		msg.run();
	}

	// update the GUI
	SetWorkingMode();
}

void BackupWindow::on_restore()
{
	Gtk::MessageDialog msg("restore");
	msg.run();
}

void BackupWindow::on_file_quit()
{
	m_dev.Disconnect();
	hide();
}

void BackupWindow::on_edit_config()
{
	ConfigDlg dlg(m_dev.GetDBDB(), *m_pConfig);
	if( dlg.run() == Gtk::RESPONSE_OK ) {
		m_pConfig->SetBackupList(dlg.GetBackupList());
		m_pConfig->SetRestoreList(dlg.GetRestoreList());
		if( !m_pConfig->Save() ) {
			Gtk::MessageDialog msg("Error saving config: " +
				m_pConfig->get_last_error());
			msg.run();
		}
	}
}

void BackupWindow::on_help_about()
{
/*
	Gtk::AboutDialog dlg;
	dlg.set_copyright("Copyright (C) 2007, Net Direct Inc.");
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

	dlg.set_authors(authors);

	int major, minor;
	const char *BarryVersion = Barry::Version(major, minor);
	dlg.set_version(std::string("Barry Backup 0.1 - Using library: ") + BarryVersion);
	dlg.set_website("http://www.netdirect.ca/");
	dlg.run();
*/
}

bool BackupWindow::on_startup()
{
	std::cout << "on_startup" << std::endl;
	if( !m_scanned ) {
		ScanAndConnect();
		m_scanned = true;
	}
	return false;
}

void BackupWindow::on_thread_progress()
{
	m_finishedRecords++;
	UpdateProgress();
}

void BackupWindow::on_thread_done()
{
	std::cout << "on_thread_done" << std::endl;

	// done!
	ClearWorkingMode();
	m_working = false;

	Gtk::MessageDialog msg("Backup complete!");
	msg.run();
}



/*
void on_showtext()
{
	Glib::ustring text = pEntry->get_text();
	Gtk::MessageDialog dialog("This is the text entered: " + text);
//		dialog.set_secondary_text(text);
	dialog.run();
}

void on_close()
{
	std::cout << "on_close()\n";
//		response(Gtk::RESPONSE_CLOSE);
//		signal_delete_event().emit();
//	Gtk::Main::quit();
	hide();
}
*/

