///
/// \file	BaseFrame.cc
///		Class for the fixed-size frame that holds the main app
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

#include "BaseFrame.h"
#include "Mode_MainMenu.h"
#include "Mode_Sync.h"
#include "ClickImage.h"
#include "barrydesktop.h"
#include "windowids.h"
#include <wx/aboutdlg.h>
#include <iostream>
#include <sstream>

// include icons and logos
#include "../images/barry_logo_icon.xpm"
#include "../images/logo_NetDirect.xpm"

using namespace std;

BEGIN_EVENT_TABLE(BaseFrame, wxFrame)
	EVT_SIZE	(BaseFrame::OnSize)
	EVT_PAINT	(BaseFrame::OnPaint)
	EVT_MOTION	(BaseFrame::OnMouseMotion)
	EVT_LEFT_DOWN	(BaseFrame::OnLeftDown)
	EVT_LEFT_UP	(BaseFrame::OnLeftUp)
	EVT_BUTTON	(MainMenu_BackupAndRestore, BaseFrame::OnBackupRestore)
	EVT_BUTTON	(MainMenu_Sync, BaseFrame::OnSync)
	EVT_BUTTON	(MainMenu_Modem, BaseFrame::OnModem)
	EVT_BUTTON	(MainMenu_BackButton, BaseFrame::OnBackButton)
	EVT_BUTTON	(HotImage_BarryLogo, BaseFrame::OnBarryLogoClicked)
	EVT_BUTTON	(HotImage_NetDirectLogo, BaseFrame::OnNetDirectLogoClicked)
	EVT_TEXT	(Ctrl_DeviceCombo, BaseFrame::OnDeviceComboChange)
	EVT_MENU	(SysMenu_VerboseLogging, BaseFrame::OnVerboseLogging)
	EVT_MENU	(SysMenu_ResetDevice, BaseFrame::OnResetDevice)
	EVT_MENU	(SysMenu_RescanUsb, BaseFrame::OnRescanUsb)
	EVT_MENU	(SysMenu_About, BaseFrame::OnAbout)
	EVT_MENU	(SysMenu_Exit, BaseFrame::OnExit)
	EVT_END_PROCESS	(Process_BackupAndRestore, BaseFrame::OnTermBackupAndRestore)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////////
// BaseFrame

BaseFrame::BaseFrame(const wxImage &background)
	: wxFrame(NULL, wxID_ANY, _T("Barry Desktop Control Panel"),
		wxPoint(50, 50),
		wxSize(background.GetWidth(), background.GetHeight()),
		wxMINIMIZE_BOX | wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU |
		wxCLIP_CHILDREN)
	, m_width(background.GetWidth())
	, m_height(background.GetHeight())
	, m_current_mode(0)
{
	// This is a workaround for different size behaviour
	// in the GTK version of wxWidgets 2.9
	SetClientSize(wxSize(background.GetWidth(), background.GetHeight()));

	// load base bitmaps
	m_background.reset( new wxBitmap(background) );

	// the main menu mode always exists, but may not always be current
	m_main_menu_mode.reset( new MainMenuMode(this) );
	m_current_mode = m_main_menu_mode.get();

	m_barry_logo.reset( new ClickableImage(this,
		wxBitmap(barry_logo_icon_xpm), HotImage_BarryLogo,
		4, 4, false) );
	wxBitmap nd_logo(logo_NetDirect_xpm);
	m_netdirect_logo.reset( new ClickableImage(this,
		nd_logo, HotImage_NetDirectLogo,
		m_width - 3 - nd_logo.GetWidth(),
		(MAIN_HEADER_OFFSET - nd_logo.GetHeight()) / 2, true,
		wxNullCursor));

	// Create the Barry Logo popup system menu
	m_sysmenu.reset( new wxMenu );
	m_sysmenu->Append( new wxMenuItem(m_sysmenu.get(),
		SysMenu_VerboseLogging, _T("&Verbose Logging"),
		_T("Enable low level USB debug output"), wxITEM_CHECK, NULL) );
	m_sysmenu->Append(SysMenu_ResetDevice, _T("Re&set Device"));
	m_sysmenu->Append(SysMenu_RescanUsb, _T("&Rescan USB"));
	m_sysmenu->AppendSeparator();
	m_sysmenu->Append(SysMenu_About, _T("&About..."));
	m_sysmenu->AppendSeparator();
	m_sysmenu->Append(wxID_EXIT, _T("E&xit"));

	UpdateMenuState();
	CreateDeviceCombo(wxGetApp().GetGlobalConfig().GetLastDevice());
}

void BaseFrame::UpdateMenuState()
{
	if( !m_sysmenu.get() )
		return;

	wxMenuItemList &list = m_sysmenu->GetMenuItems();
	wxMenuItemList::iterator b = list.begin();
	for( ; b != list.end(); ++b ) {
		wxMenuItem *item = *b;

		switch( item->GetId() )
		{
		case SysMenu_VerboseLogging:
			item->Check(Barry::IsVerbose());
			break;
		}
	}
}

void BaseFrame::CreateDeviceCombo(Barry::Pin pin)
{
	const Barry::Probe::Results &results = wxGetApp().GetResults();

	// default to:
	//	no device selected, if multiple available and no default given
	//	no devices available, if nothing there
	//	the first device, if only one exists
	int selected = 0;

	// create a list of selections
	wxArrayString devices;

	// if there's more than one device, let the user pick "none"
	if( results.size() > 1 ) {
		devices.Add(_T("No device selected"));
	}

	// add one entry for each device
	for( Barry::Probe::Results::const_iterator i = results.begin();
				i != results.end(); ++i ) {
		std::ostringstream oss;
		oss << i->m_pin.str();
		if( i->m_cfgDeviceName.size() ) {
			oss << " (" << i->m_cfgDeviceName << ")";
		}

		// if this is the desired item, remember this selection
		if( pin.valid() && i->m_pin == pin ) {
			selected = devices.GetCount();
		}

		devices.Add(wxString(oss.str().c_str(), wxConvUTF8));
	}

	// if nothing is there, be descriptive
	if( devices.GetCount() == 0 ) {
		devices.Add(_T("No devices available"));
	}

	// create the combobox
	int x = m_width - 300;
	int y = m_height - (MAIN_HEADER_OFFSET - 5);
	m_device_combo.reset( new wxComboBox(this, Ctrl_DeviceCombo, _T(""),
		wxPoint(x, y), wxSize(290, -1), devices, wxCB_READONLY) );

	// select the desired entry
	m_device_combo->SetValue(devices[selected]);

	// update the screenshot
	m_main_menu_mode->UpdateScreenshot(GetCurrentComboPin());
}

Barry::Pin BaseFrame::GetCurrentComboPin()
{
	// fetch newly selected device
	wxString value = m_device_combo->GetValue();
	istringstream iss(string(value.utf8_str()).substr(0,8));
	Barry::Pin pin;
	iss >> pin;
	return pin;
}

void BaseFrame::EnableBackButton(Mode *new_mode)
{
	// create the button - this goes in the bottom FOOTER area
	// so the height must be fixed to MAIN_HEADER_OFFSET
	// minus a border of 5px top and bottom
	wxPoint pos(10, m_height - (MAIN_HEADER_OFFSET - 5));
	wxSize size(-1, MAIN_HEADER_OFFSET - 5 - 5);

	m_back_button.reset( new wxButton(this, MainMenu_BackButton,
		_T("Main Menu"), pos, size) );

	// set the new mode
	m_current_mode = new_mode;

	// destroy the device switcher combo box
	m_device_combo.reset();

	// repaint!
	Refresh(false);
}

void BaseFrame::DisableBackButton()
{
	// destroy the back button
	m_back_button.reset();

	// delete all modes
	m_sync_mode.reset();

	// create the device switcher combo again
	CreateDeviceCombo(wxGetApp().GetGlobalConfig().GetLastDevice());

	// reset the current mode to main menu and repaint
	m_current_mode = m_main_menu_mode.get();
	Refresh(false);
}

void BaseFrame::OnSize(wxSizeEvent &event)
{
}

void BaseFrame::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);
	dc.SetMapMode(wxMM_TEXT);

	// paint the background image
	dc.DrawBitmap(*m_background, 0, 0);

	// paint the header: Barry logo
	m_barry_logo->Draw(dc);

	// paint the header: NetDirect logo
	m_netdirect_logo->Draw(dc);

	// paint the header: text
	auto_ptr<wxFont> font( wxFont::New(14,
		wxFONTFAMILY_SWISS, wxFONTFLAG_ANTIALIASED,
		_T("Luxi Sans")) );
	dc.SetFont( *font );
	dc.SetTextForeground( wxColour(0xd2, 0xaf, 0x0b) );
	dc.SetTextBackground( wxColour(0, 0, 0, wxALPHA_TRANSPARENT) );

	long width, height, descent;
	wxString header = _T("Barry Desktop Control Panel");
	if( m_current_mode )
		header = m_current_mode->GetTitleText();
	dc.GetTextExtent(header, &width, &height, &descent);
	int x = (m_width - width) / 2;
	int y = (MAIN_HEADER_OFFSET - height) / 2;
	dc.DrawText(header, x, y);

	// let the mode do its thing
	if( m_current_mode )
		m_current_mode->OnPaint(dc);
}

void BaseFrame::OnMouseMotion(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_barry_logo->HandleMotion(dc, event.m_x, event.m_y);
	m_netdirect_logo->HandleMotion(dc, event.m_x, event.m_y);

	// the mode
	if( m_current_mode )
		m_current_mode->OnMouseMotion(dc, event.m_x, event.m_y);
}

void BaseFrame::OnLeftDown(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_barry_logo->HandleDown(dc, event.m_x, event.m_y);
	m_netdirect_logo->HandleDown(dc, event.m_x, event.m_y);
	event.Skip();

	// the mode
	if( m_current_mode )
		m_current_mode->OnLeftDown(dc, event.m_x, event.m_y);
}

void BaseFrame::OnLeftUp(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_barry_logo->HandleUp(dc, event.m_x, event.m_y);
	m_netdirect_logo->HandleUp(dc, event.m_x, event.m_y);

	// the mode
	if( m_current_mode )
		m_current_mode->OnLeftUp(dc, event.m_x, event.m_y);
}

void BaseFrame::OnBackupRestore(wxCommandEvent &event)
{
	if( m_backup_process.get() ) {
		wxMessageBox(_T("The Backup program is already running!"), _T("Backup and Restore"), wxOK | wxICON_INFORMATION);
		return;
	}

	m_backup_process.reset( new StatusProcess(this, Process_BackupAndRestore) );
	const wxChar *argv[] = {
		_T("barrybackup"),
		NULL
	};
	long ret = wxExecute((wxChar**)argv, wxEXEC_ASYNC, m_backup_process.get());
	cout << "wxExecute returned " << ret << endl;
	if( ret == 0 ) {
		m_backup_process.reset();
		wxMessageBox(_T("Failed to run barrybackup. Please make sure it is installed and in your PATH."), _T("Backup and Restore"), wxOK | wxICON_ERROR);
	}
}

void BaseFrame::OnSync(wxCommandEvent &event)
{
	if( wxGetApp().GetOpenSync().GetAvailable() == 0 ) {
		wxGetApp().ShowMissingOpenSyncMessage();
		return;
	}

	try {
		m_sync_mode.reset( new SyncMode(this) );
	}
	catch( std::exception &e ) {
		wxString msg(_T(
			"An error occurred that prevented the loading of Sync\n"
			"mode.  This is most likely because a critical piece\n"
			"of OpenSync is missing.  Check that all required\n"
			"plugins are installed, and that tools like 'bidentify'\n"
			"can find your BlackBerry successfully.\n\n"
			"Error: "));
		msg += wxString(e.what(), wxConvUTF8);
		wxMessageBox(msg, _T("Sync Mode"), wxOK | wxICON_ERROR);
		return;
	}

	EnableBackButton(m_sync_mode.get());
}

void BaseFrame::OnModem(wxCommandEvent &event)
{
}

void BaseFrame::OnBackButton(wxCommandEvent &event)
{
	DisableBackButton();
}

void BaseFrame::OnTermBackupAndRestore(wxProcessEvent &event)
{
	cout << "OnTermBackupAndRestore(): done = "
		<< (m_backup_process->IsDone() ? "true" : "false")
		<< ", status = " << m_backup_process->GetStatus()
		<< endl;
	if( m_backup_process->IsDone() && m_backup_process->GetStatus() ) {
		wxMessageBox(_T("Unable to run barrybackup, or it returned an error. Please make sure it is installed and in your PATH."), _T("Backup and Restore"), wxOK | wxICON_ERROR);
	}
	m_backup_process.reset();
}

void BaseFrame::OnBarryLogoClicked(wxCommandEvent &event)
{
	PopupMenu(m_sysmenu.get(), 20, 20);
}

void BaseFrame::OnNetDirectLogoClicked(wxCommandEvent &event)
{
	// fire up a browser to point to the Barry documentation
	wxBusyCursor wait;
	::wxLaunchDefaultBrowser(_T("http://netdirect.ca/barry"));
}

void BaseFrame::OnDeviceComboChange(wxCommandEvent &event)
{
	Barry::Pin pin = GetCurrentComboPin();

	// any change?
	if( pin == wxGetApp().GetGlobalConfig().GetLastDevice() )
		return;	// nope

	// save
	wxGetApp().GetGlobalConfig().SetLastDevice(pin);

	// update the main mode's screenshot
	m_main_menu_mode->UpdateScreenshot(pin);
	if( m_current_mode == m_main_menu_mode.get() )
		Refresh(false);

	// FIXME - if inside a sub menu mode, we need to destroy the mode
	// class and start fresh
}

void BaseFrame::OnVerboseLogging(wxCommandEvent &event)
{
	Barry::Verbose( !Barry::IsVerbose() );
	wxGetApp().GetGlobalConfig().SetVerboseLogging( Barry::IsVerbose() );
	UpdateMenuState();
}

void BaseFrame::OnResetDevice(wxCommandEvent &event)
{
	int i = Barry::Probe::Find(wxGetApp().GetResults(), GetCurrentComboPin());
	if( i != -1 ) {
		Usb::Device dev(wxGetApp().GetResults()[i].m_dev);
		dev.Reset();
		wxBusyCursor wait;
		wxSleep(4);
		OnRescanUsb(event);
	}
}

void BaseFrame::OnRescanUsb(wxCommandEvent &event)
{
	if( m_current_mode == m_main_menu_mode.get() ) {
		std::auto_ptr<UsbScanSplash> splash( new UsbScanSplash );
		wxGetApp().Probe();
		CreateDeviceCombo(wxGetApp().GetGlobalConfig().GetLastDevice());
	}
	else {
		// FIXME - tell the user we didn't do anything?
		// or perhaps just disable rescan while in a mode
	}
}

void BaseFrame::OnAbout(wxCommandEvent &event)
{
	wxAboutDialogInfo info;
	info.SetName(_T("Barry Desktop Control Panel"));
	info.SetVersion(_T("0.17"));
	info.SetDescription(_T("A Free Software graphical user interface for working with the BlackBerry(TM) handheld."));
	info.SetCopyright(_T("Copyright (C) 2009-2010, Net Direct Inc."));
	info.SetWebSite(_T("http://netdirect.ca/barry"));
	info.SetLicense(_T(
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
"    root directory of this project for more details.\n"));

	info.AddDeveloper(_T("Net Direct Inc."));
//	info.AddDeveloper(_T("Chris Frey <cdfrey@foursquare.net>"));
//	info.AddDeveloper(_T("See AUTHORS file for detailed"));
//	info.AddDeveloper(_T("contribution information."));

	info.AddArtist(_T("Chris Frey - GUI interface"));
	info.AddArtist(_T("Martin Owens - Barry logo"));
	info.AddArtist(_T("Tango Desktop Project - Public domain icons"));

	wxAboutBox(info);
}

void BaseFrame::OnExit(wxCommandEvent &event)
{
	Close(true);
}

