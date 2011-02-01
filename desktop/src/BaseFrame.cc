///
/// \file	BaseFrame.cc
///		Class for the fixed-size frame that holds the main app
///

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "Mode_Browse.h"
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
	EVT_BUTTON	(MainMenu_AppLoader, BaseFrame::OnAppLoader)
	EVT_BUTTON	(MainMenu_DeviceSwitch, BaseFrame::OnDeviceSwitch)
	EVT_BUTTON	(MainMenu_BrowseDatabases, BaseFrame::OnBrowseDatabases)
	EVT_BUTTON	(MainMenu_MediaManagement, BaseFrame::OnMediaManagement)
	EVT_BUTTON	(MainMenu_Misc, BaseFrame::OnMisc)
	EVT_BUTTON	(MainMenu_BackButton, BaseFrame::OnBackButton)
	EVT_BUTTON	(HotImage_BarryLogo, BaseFrame::OnBarryLogoClicked)
	EVT_BUTTON	(HotImage_NetDirectLogo, BaseFrame::OnNetDirectLogoClicked)
	EVT_TEXT	(Ctrl_DeviceCombo, BaseFrame::OnDeviceComboChange)
	EVT_MENU	(SysMenu_VerboseLogging, BaseFrame::OnVerboseLogging)
	EVT_MENU	(SysMenu_RenameDevice, BaseFrame::OnRenameDevice)
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
	, TermCatcher(this, Process_BackupAndRestore)
	, m_width(background.GetWidth())
	, m_height(background.GetHeight())
	, m_current_mode(0)
	, m_backup_process(this)
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
	m_sysmenu->Append(SysMenu_RenameDevice, _T("Re&name Device..."));
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
		oss << i->m_pin.Str();
		if( i->m_cfgDeviceName.size() ) {
			oss << " (" << i->m_cfgDeviceName << ")";
		}

		// if this is the desired item, remember this selection
		if( pin.Valid() && i->m_pin == pin ) {
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

	// without the device combo, there is no concept of a
	// "current device" so temporarily disable the USB options
	m_sysmenu->Enable(SysMenu_RenameDevice, false);
	m_sysmenu->Enable(SysMenu_ResetDevice, false);
	m_sysmenu->Enable(SysMenu_RescanUsb, false);

	// repaint!
	Refresh(false);
}

void BaseFrame::DisableBackButton()
{
	// destroy the back button
	m_back_button.reset();

	// delete all modes
	m_sync_mode.reset();
	m_browse_mode.reset();

	// create the device switcher combo again
	CreateDeviceCombo(wxGetApp().GetGlobalConfig().GetLastDevice());

	// enable the USB menu options
	Barry::Pin pin = GetCurrentComboPin();
	m_sysmenu->Enable(SysMenu_RenameDevice, pin.Valid());
	m_sysmenu->Enable(SysMenu_ResetDevice, true);
	m_sysmenu->Enable(SysMenu_RescanUsb, true);

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
//	m_netdirect_logo->Draw(dc);

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
//	m_netdirect_logo->HandleMotion(dc, event.m_x, event.m_y);

	// the mode
	if( m_current_mode )
		m_current_mode->OnMouseMotion(dc, event.m_x, event.m_y);
}

void BaseFrame::OnLeftDown(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_barry_logo->HandleDown(dc, event.m_x, event.m_y);
//	m_netdirect_logo->HandleDown(dc, event.m_x, event.m_y);
	event.Skip();

	// the mode
	if( m_current_mode )
		m_current_mode->OnLeftDown(dc, event.m_x, event.m_y);
}

void BaseFrame::OnLeftUp(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_barry_logo->HandleUp(dc, event.m_x, event.m_y);
//	m_netdirect_logo->HandleUp(dc, event.m_x, event.m_y);

	// the mode
	if( m_current_mode )
		m_current_mode->OnLeftUp(dc, event.m_x, event.m_y);
}

void BaseFrame::OnBackupRestore(wxCommandEvent &event)
{
	if( m_backup_process.IsAppRunning() ) {
		wxMessageBox(_T("The Backup program is already running!"),
			_T("Backup and Restore"), wxOK | wxICON_INFORMATION);
		return;
	}

	if( !m_backup_process.Run(this, "Backup and Restore", _T("barrybackup")) )
		return;
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

//#include "ConflictDlg.h"
void BaseFrame::OnModem(wxCommandEvent &event)
{
/*
	OpenSync::SyncChange change;
	change.id = 1;
	change.member_id = 1;
	change.plugin_name = "barry-sync";
	change.uid = "12341524235234";
	change.printable_data =
"<contact>\n"
"  <UnknownNode>\n"
"    <NodeName>PRODID</NodeName>\n"
"    <Content>-//OpenSync//NONSGML Barry Contact Record//EN</Content>\n"
"  </UnknownNode>\n"
"  <FormattedName>\n"
"    <Content>Adame Brandee</Content>\n"
"  </FormattedName>\n"
"  <Name>\n"
"    <LastName>Brandee</LastName>\n"
"    <FirstName>Adame</FirstName>\n"
"  </Name>\n"
"  <AddressLabel>\n"
"    <Content>71 Long St.\n"
"Toronto ON Canada\n"
"N0N 0N0</Content>\n"
"    <Type>home</Type>\n"
"  </AddressLabel>\n"
"  <Address>\n"
"    <Street>71 Long St.</Street>\n"
"    <City>Toronto</City>\n"
"    <Region>ON</Region>\n"
"    <PostalCode>N0N 0N0</PostalCode>\n"
"    <Country>Canada</Country>\n"
"    <Type>home</Type>\n"
"  </Address>\n"
"  <Telephone>\n"
"    <Content>+1 (416) 555-7711</Content>\n"
"    <Type>voice</Type>\n"
"    <Type>home</Type>\n"
"  </Telephone>\n"
"  <Telephone>\n"
"    <Content>+1 (416) 955-7117</Content>\n"
"    <Type>msg</Type>\n"
"    <Type>cell</Type>\n"
"  </Telephone>\n"
"  <EMail>\n"
"    <Content>abrandee@sympatico.ca</Content>\n"
"    <Type>internet</Type>\n"
"    <Type>pref</Type>\n"
"  </EMail>\n"
"  <Categories>\n"
"    <Category>Personal</Category>\n"
"  </Categories>\n"
"  <Note>\n"
"    <Content>Interweb salesman... 24/7</Content>\n"
"  </Note>\n"
"</contact>";

	std::vector<OpenSync::SyncChange> changes;
	changes.push_back(change);

	change.id = 2;
	change.member_id = 2;
	change.plugin_name = "evo2-sync";
	change.uid = "asdfioausdf_1235234as_asdf12341524235234";
	change.printable_data =
"<contact>\n"
"  <Telephone>\n"
"    <Content>+1 (416) 955-7117</Content>\n"
"    <Type>CELL</Type>\n"
"    <Slot>2</Slot>\n"
"  </Telephone>\n"
"  <Telephone>\n"
"    <Content>+1 (416) 555-7711</Content>\n"
"    <Type>HOME</Type>\n"
"    <Type>VOICE</Type>\n"
"    <Slot>1</Slot>\n"
"  </Telephone>\n"
"  <EMail>\n"
"    <Content>abrandee@sympatico.ca</Content>\n"
"    <Type>OTHER</Type>\n"
"    <Slot>1</Slot>\n"
"  </EMail>\n"
"  <WantsHtml>\n"
"    <Content>FALSE</Content>\n"
"  </WantsHtml>\n"
"  <Revision>\n"
"    <Content>20100322T225303Z</Content>\n"
"  </Revision>\n"
"  <UnknownNode>\n"
"    <NodeName>PRODID</NodeName>\n"
"    <Content>-//OpenSync//NONSGML Barry Contact Record//EN</Content>\n"
"  </UnknownNode>\n"
"  <FormattedName>\n"
"    <Content>Adam Brandeee</Content>\n"
"  </FormattedName>\n"
"  <Name>\n"
"    <LastName>Brandeee</LastName>\n"
"    <FirstName>Adam</FirstName>\n"
"  </Name>\n"
"  <AddressLabel>\n"
"    <Content>71 Long St.\n"
"Toronto, ON\n"
"N0N 0N1\n"
"Canada</Content>\n"
"    <Type>home</Type>\n"
"  </AddressLabel>\n"
"  <Address>\n"
"    <Street>71 Long St.</Street>\n"
"    <City>Toronto</City>\n"
"    <Region>ON</Region>\n"
"    <PostalCode>N0N 0N1</PostalCode>\n"
"    <Country>Canada</Country>\n"
"    <Type>home</Type>\n"
"  </Address>\n"
"  <Categories>\n"
"    <Category>Personal</Category>\n"
"  </Categories>\n"
"  <FileAs>\n"
"    <Content>Brandeee, Adam</Content>\n"
"  </FileAs>\n"
"</contact>";

	changes.push_back(change);

	{
		ConflictDlg::AlwaysMemoryBlock always;
		ConflictDlg dlg(this, *wxGetApp().GetOpenSync().os22(),
			"SDAIN", changes, always);
		dlg.ShowModal();
		wxString msg(dlg.GetCommand().c_str(), wxConvUTF8);
		msg += _T(" ");
		msg += always.m_always ? _T("always") : _T("not always");
		wxMessageBox(msg);
	}
*/
}

void BaseFrame::OnAppLoader(wxCommandEvent &event)
{
/*
	OpenSync::SyncChange change;
	change.id = 1;
	change.member_id = 1;
	change.plugin_name = "barry-sync";
	change.uid = "12341524235234";
	change.printable_data =
"<vcal>\n"
"  <Event>\n"
"    <Sequence>\n"
"      <Content>0</Content>\n"
"    </Sequence>\n"
"    <Summary>\n"
"      <Content>Subject</Content>\n"
"    </Summary>\n"
"    <Description>\n"
"      <Content>Bring burnt offering</Content>\n"
"    </Description>\n"
"    <Location>\n"
"      <Content>Tent</Content>\n"
"    </Location>\n"
"    <DateStarted>\n"
"      <Content>20100506T040000Z</Content>\n"
"    </DateStarted>\n"
"    <DateEnd>\n"
"      <Content>20100507T040000Z</Content>\n"
"    </DateEnd>\n"
"    <Alarm>\n"
"      <AlarmAction>AUDIO</AlarmAction>\n"
"      <AlarmTrigger>\n"
"        <Content>20100506T034500Z</Content>\n"
"        <Value>DATE-TIME</Value>\n"
"      </AlarmTrigger>\n"
"    </Alarm>\n"
"  </Event>\n"
"</vcal>\n";

	std::vector<OpenSync::SyncChange> changes;
	changes.push_back(change);

	change.id = 2;
	change.member_id = 2;
	change.plugin_name = "evo2-sync";
	change.uid = "asdfioausdf_1235234as_asdf12341524235234";
	change.printable_data =
"<vcal>\n"
"  <Method>\n"
"    <Content>PUBLISH</Content>\n"
"  </Method>\n"
"  <Timezone>\n"
"    <TimezoneID>/softwarestudio.org/Tzfile/America/Thunder_Bay</TimezoneID>\n"
"    <Location>America/Thunder_Bay</Location>\n"
"    <Standard>\n"
"      <TimezoneName>EST</TimezoneName>\n"
"      <DateStarted>19701107T010000</DateStarted>\n"
"      <RecurrenceRule>\n"
"        <Rule>FREQ=YEARLY</Rule>\n"
"        <Rule>INTERVAL=1</Rule>\n"
"        <Rule>BYDAY=2SU</Rule>\n"
"        <Rule>BYMONTH=11</Rule>\n"
"      </RecurrenceRule>\n"
"      <TZOffsetFrom>-0400</TZOffsetFrom>\n"
"      <TZOffsetTo>-0500</TZOffsetTo>\n"
"    </Standard>\n"
"    <DaylightSavings>\n"
"      <TimezoneName>EDT</TimezoneName>\n"
"      <DateStarted>19700313T030000</DateStarted>\n"
"      <RecurrenceRule>\n"
"        <Rule>FREQ=YEARLY</Rule>\n"
"        <Rule>INTERVAL=1</Rule>\n"
"        <Rule>BYDAY=2SU</Rule>\n"
"        <Rule>BYMONTH=3</Rule>\n"
"      </RecurrenceRule>\n"
"      <TZOffsetFrom>-0500</TZOffsetFrom>\n"
"      <TZOffsetTo>-0400</TZOffsetTo>\n"
"    </DaylightSavings>\n"
"  </Timezone>\n"
"  <Event>\n"
"    <Sequence>\n"
"      <Content>1</Content>\n"
"    </Sequence>\n"
"    <Summary>\n"
"      <Content>Celebration day</Content>\n"
"    </Summary>\n"
"    <Location>\n"
"      <Content>Tent of</Content>\n"
"    </Location>\n"
"    <DateStarted>\n"
"      <Content>20100506T000000</Content>\n"
"      <TimezoneID>/softwarestudio.org/Tzfile/America/Thunder_Bay</TimezoneID>\n"
"    </DateStarted>\n"
"    <DateEnd>\n"
"      <Content>20100507T000000</Content>\n"
"      <TimezoneID>/softwarestudio.org/Tzfile/America/Thunder_Bay</TimezoneID>\n"
"    </DateEnd>\n"
"    <DateCalendarCreated>\n"
"      <Content>20100430T214736Z</Content>\n"
"    </DateCalendarCreated>\n"
"    <DateCreated>\n"
"      <Content>20100430T214736</Content>\n"
"    </DateCreated>\n"
"    <LastModified>\n"
"      <Content>20100430T214927</Content>\n"
"    </LastModified>\n"
"    <Description>\n"
"      <Content>Bring burnt offering</Content>\n"
"    </Description>\n"
"    <Class>\n"
"      <Content>PUBLIC</Content>\n"
"    </Class>\n"
"    <Transparency>\n"
"      <Content>OPAQUE</Content>\n"
"    </Transparency>\n"
"    <Alarm>\n"
"      <AlarmAction>AUDIO</AlarmAction>\n"
"      <AlarmTrigger>\n"
"        <Content>20100506T034500Z</Content>\n"
"        <Value>DATE-TIME</Value>\n"
"      </AlarmTrigger>\n"
"    </Alarm>\n"
"  </Event>\n"
"</vcal>\n";


	changes.push_back(change);

	{
		ConflictDlg::AlwaysMemoryBlock always;
		ConflictDlg dlg(this, *wxGetApp().GetOpenSync().os22(),
			"SDAIN", changes, always);
		dlg.ShowModal();
		wxString msg(dlg.GetCommand().c_str(), wxConvUTF8);
		msg += _T(" ");
		msg += always.m_always ? _T("always") : _T("not always");
		wxMessageBox(msg);
	}
*/
}

void BaseFrame::OnDeviceSwitch(wxCommandEvent &event)
{
}

void BaseFrame::OnBrowseDatabases(wxCommandEvent &event)
{
	int i = Barry::Probe::Find(wxGetApp().GetResults(), GetCurrentComboPin());
	if( i == -1 ) {
		wxMessageBox(_T("There is no device selected in the device list.  Please select a device to browse."),
			_T("Database Browser Mode"), wxOK | wxICON_ERROR);
		return;
	}

	try {
		m_browse_mode.reset( new BrowseMode(this,
				wxGetApp().GetResults()[i]) );
	}
	catch( std::exception &e ) {
		wxString msg(_T(
			"An error occurred that prevented the loading of Database\n"
			"Browse mode.  This could be due to a low level USB\n"
			"issue.  Please make sure your device is plugged in\n"
			"and not in Desktop Mode.  If it is, try replugging\n"
			"the device, and rescanning the USB bus from the menu.\n"
			"\n"
			"Error: "));
		msg += wxString(e.what(), wxConvUTF8);
		wxMessageBox(msg, _T("Database Browser Mode"), wxOK | wxICON_ERROR);
		return;
	}

	EnableBackButton(m_browse_mode.get());
}

void BaseFrame::OnMediaManagement(wxCommandEvent &event)
{
}

void BaseFrame::OnMisc(wxCommandEvent &event)
{
}

void BaseFrame::OnBackButton(wxCommandEvent &event)
{
	DisableBackButton();
}

void BaseFrame::OnTermBackupAndRestore(wxProcessEvent &event)
{
	barryverbose("OnTermBackupAndRestore(): done = "
		<< (!m_backup_process.IsAppRunning() ? "true" : "false")
		<< ", status = " << m_backup_process.GetAppStatus());
	if( !m_backup_process.IsAppRunning() &&
	    m_backup_process.GetAppStatus() )
	{
		wxMessageBox(_T("Unable to run barrybackup, or it returned an error. Please make sure it is installed and in your PATH."),
			_T("Backup and Restore"), wxOK | wxICON_ERROR);
	}
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

	// update sys menu
	m_sysmenu->Enable(SysMenu_RenameDevice, false);

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

void BaseFrame::OnRenameDevice(wxCommandEvent &event)
{
	Barry::Pin pin = GetCurrentComboPin();
	if( !pin.Valid() )
		return;

	// grab the current known name of the device
	const Barry::Probe::Results &results = wxGetApp().GetResults();
	int index = Barry::Probe::Find(results, pin);
	if( index == -1 )
		return;

	wxString current_name(results[index].m_cfgDeviceName.c_str(), wxConvUTF8);
	wxTextEntryDialog dlg(this,
		_T("Please enter a name for the current device:"),
		_T("Rename Device"),
		current_name, wxTextEntryDialogStyle);

	if( dlg.ShowModal() != wxID_OK )
		return; // nothing to do
	wxString name = dlg.GetValue();
	if( name == current_name )
		return; // nothing to do

	wxGetApp().SetDeviceName(pin, string(name.utf8_str()));

	// refill combo box
	CreateDeviceCombo(wxGetApp().GetGlobalConfig().GetLastDevice());
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
	info.SetCopyright(_T("Copyright (C) 2009-2011, Net Direct Inc."));
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

