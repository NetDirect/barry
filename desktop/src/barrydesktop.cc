///
/// \file	barrydesktop.cc
///		Program entry point for the desktop GUI
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

#include <iostream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <barry/barry.h>
#include <wx/wx.h>
#include <wx/aboutdlg.h>
#include <wx/splash.h>
#include <wx/process.h>
#include <wx/mstream.h>
#include "windowids.h"
#include "os22.h"
#include "os40.h"
#include "deviceset.h"
#include "GroupCfgDlg.h"

// include icons and logos
#include "../images/barry_logo_icon.xpm"
#include "../images/logo_NetDirect.xpm"

using namespace std;

#define MAIN_HEADER_OFFSET 40

const wxChar *ButtonNames[] = {
	_T("backuprestore"),
	_T("sync"),
	_T("modem"),
	_T("apploader"),
	_T("deviceswitch"),
	_T("browsedatabases"),
	_T("media"),
	_T("misc"),
	0
	};

const wxChar *StateNames[] = {
	_T("-normal.png"),
	_T("-focus.png"),
	_T("-pushed.png"),
	0
	};

#define BUTTON_STATE_NORMAL	0
#define BUTTON_STATE_FOCUS	1
#define BUTTON_STATE_PUSHED	2

wxString GetImageFilename(const wxString &filename);
wxString GetButtonFilename(int id, int state);

class StatusProcess : public wxProcess
{
	bool m_done;
	int m_status;

public:
	StatusProcess(wxWindow *parent, int id)
		: wxProcess(parent, id)
		, m_done(false)
		, m_status(0)
	{
	}

	bool IsDone() const { return m_done; }
	int GetStatus() const { return m_status; }

	virtual void OnTerminate(int pid, int status)
	{
		m_status = status;
		m_done = true;
		wxProcess::OnTerminate(pid, status);
	}
};

class ClickableImage
{
	wxWindow *m_parent;
	int m_id;
	wxBitmap m_image;
	int m_x, m_y;
	bool m_focus;
	bool m_event_on_up;
	wxCursor m_hover_cursor;

protected:
	bool CalculateHit(int x, int y);

public:
	ClickableImage(wxWindow *parent, const wxBitmap &image,
		int ID, int x, int y, bool event_on_up = true,
		const wxCursor &hover = wxCursor(wxCURSOR_HAND));

	void Draw(wxDC &dc);
	void HandleMotion(wxDC &dc, int x, int y);
	void HandleDown(wxDC &dc, int x, int y);
	void HandleUp(wxDC &dc, int x, int y);
};

class PNGButton
{
	wxBitmap m_bitmaps[3]; // normal[0], focus[1], pushed[2]
	wxBitmap m_background;
	wxWindow *m_parent;
	int m_id;
	int m_x, m_y;
	int m_state;	// index into m_bitmaps

protected:
	wxBitmap LoadButtonBitmap(int state);

public:
	PNGButton(wxWindow *parent, int ID, int x, int y);

	bool IsPushed() const { return m_state == BUTTON_STATE_PUSHED; }

	void Init(wxDC &dc);
	void Draw(wxDC &dc);
	void Normal(wxDC &dc);
	void Focus(wxDC &dc);
	void Push(wxDC &dc);
	void Click(wxDC &dc);
};

class BaseButtons
{
private:
	std::vector<PNGButton*> m_buttons;
	PNGButton *m_current;
	int m_buttonWidth, m_buttonHeight;

protected:
	PNGButton* CalculateHit(int x, int y);

public:
	BaseButtons(wxWindow *parent);
	~BaseButtons();

	void InitAll(wxDC &dc);
	void DrawAll(wxDC &dc);
	void HandleMotion(wxDC &dc, int x, int y);
	void HandleDown(wxDC &dc, int x, int y);
	void HandleUp(wxDC &dc, int x, int y);
};

class Mode
{
public:
	Mode() {}
	virtual ~Mode() {}

	// events (called from BaseFrame)
	virtual wxString GetTitleText() const { return _T("FIXME"); }
	virtual void OnPaint(wxDC &dc) {}
	virtual void OnMouseMotion(wxDC &dc, int x, int y) {}
	virtual void OnLeftDown(wxDC &dc, int x, int y) {}
	virtual void OnLeftUp(wxDC &dc, int x, int y) {}
};

class MainMenuMode : public Mode
{
	std::auto_ptr<BaseButtons> m_basebuttons;
	wxBitmap m_screenshot;

public:
	MainMenuMode(wxWindow *parent);

	void UpdateScreenshot(const Barry::Pin &pin);

	// events (called from BaseFrame)
	wxString GetTitleText() const
	{
		return _T("Barry Desktop Control Panel");
	}

	void OnPaint(wxDC &dc);
	void OnMouseMotion(wxDC &dc, int x, int y);
	void OnLeftDown(wxDC &dc, int x, int y);
	void OnLeftUp(wxDC &dc, int x, int y);
};

class SyncMode : public wxEvtHandler, public Mode
{
private:
	DECLARE_EVENT_TABLE()

private:
	wxWindow *m_parent;

	std::auto_ptr<DeviceSet> m_device_set;

	// window controls
	std::auto_ptr<wxButton> m_sync_now_button;
	std::auto_ptr<wxListCtrl> m_device_list;
	std::auto_ptr<wxStaticText> m_label;
	std::auto_ptr<wxStaticBox> m_box;

protected:
	void FillDeviceList();

public:
	SyncMode(wxWindow *parent);
	~SyncMode();

	// virtual override events (derived from Mode)
	wxString GetTitleText() const { return _T("Barry Sync"); }

	// window events
	void OnSyncNow(wxCommandEvent &event);
	void OnConfigureDevice(wxListEvent &event);
};

BEGIN_EVENT_TABLE(SyncMode, wxEvtHandler)
	EVT_BUTTON	(SyncMode_SyncNowButton, SyncMode::OnSyncNow)
	EVT_LIST_ITEM_ACTIVATED(SyncMode_DeviceList, SyncMode::OnConfigureDevice)
END_EVENT_TABLE()

class BaseFrame : public wxFrame
{
private:
	DECLARE_EVENT_TABLE()

private:
	std::auto_ptr<wxBitmap> m_background;
	std::auto_ptr<MainMenuMode> m_main_menu_mode;// only this mode is
							// never reset()
	std::auto_ptr<SyncMode> m_sync_mode;
	std::auto_ptr<ClickableImage> m_barry_logo, m_netdirect_logo;
	std::auto_ptr<wxMenu> m_sysmenu;
	std::auto_ptr<wxComboBox> m_device_combo;
	std::auto_ptr<StatusProcess> m_backup_process;
	std::auto_ptr<wxButton> m_back_button;
	int m_width, m_height;

	Mode *m_current_mode;

public:
	BaseFrame(const wxImage &background);

	// utility functions
	void UpdateMenuState();
	void CreateDeviceCombo(Barry::Pin pin = Barry::Pin());
	Barry::Pin GetCurrentComboPin();
	void EnableBackButton(Mode *new_mode);
	void DisableBackButton();	// also returns to the main menu

	// events
	void OnSize(wxSizeEvent &event);
	void OnPaint(wxPaintEvent &event);
	void OnMouseMotion(wxMouseEvent &event);
	void OnLeftDown(wxMouseEvent &event);
	void OnLeftUp(wxMouseEvent &event);
	void OnBackupRestore(wxCommandEvent &event);
	void OnSync(wxCommandEvent &event);
	void OnBackButton(wxCommandEvent &event);
	void OnTermBackupAndRestore(wxProcessEvent &event);
	void OnBarryLogoClicked(wxCommandEvent &event);
	void OnNetDirectLogoClicked(wxCommandEvent &event);
	void OnDeviceComboChange(wxCommandEvent &event);

	// sys menu (triggered by the Barry logo)
	void OnVerboseLogging(wxCommandEvent &event);
	void OnRescanUsb(wxCommandEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnExit(wxCommandEvent &event);
};

BEGIN_EVENT_TABLE(BaseFrame, wxFrame)
	EVT_SIZE	(BaseFrame::OnSize)
	EVT_PAINT	(BaseFrame::OnPaint)
	EVT_MOTION	(BaseFrame::OnMouseMotion)
	EVT_LEFT_DOWN	(BaseFrame::OnLeftDown)
	EVT_LEFT_UP	(BaseFrame::OnLeftUp)
	EVT_BUTTON	(MainMenu_BackupAndRestore, BaseFrame::OnBackupRestore)
	EVT_BUTTON	(MainMenu_Sync, BaseFrame::OnSync)
	EVT_BUTTON	(MainMenu_BackButton, BaseFrame::OnBackButton)
	EVT_BUTTON	(HotImage_BarryLogo, BaseFrame::OnBarryLogoClicked)
	EVT_BUTTON	(HotImage_NetDirectLogo, BaseFrame::OnNetDirectLogoClicked)
	EVT_TEXT	(Ctrl_DeviceCombo, BaseFrame::OnDeviceComboChange)
	EVT_MENU	(SysMenu_VerboseLogging, BaseFrame::OnVerboseLogging)
	EVT_MENU	(SysMenu_RescanUsb, BaseFrame::OnRescanUsb)
	EVT_MENU	(SysMenu_About, BaseFrame::OnAbout)
	EVT_MENU	(SysMenu_Exit, BaseFrame::OnExit)
	EVT_END_PROCESS	(Process_BackupAndRestore, BaseFrame::OnTermBackupAndRestore)
END_EVENT_TABLE()

class BarryDesktopApp : public wxApp
{
private:
	Barry::GlobalConfigFile m_global_config;
	Barry::Probe::Results m_results;
	std::auto_ptr<OpenSync::APISet> m_set;

public:
	BarryDesktopApp();

	//
	// data access
	//
	Barry::GlobalConfigFile& GetGlobalConfig() { return m_global_config; }
	const Barry::Probe::Results& GetResults() const { return m_results; }
	OpenSync::APISet& GetOpenSync() { return *m_set; }

	//
	// operations
	//

	void ShowMissingOpenSyncMessage();

	/// Fills m_results with new data after a brand new scan.
	/// Does not catch exceptions.
	void Probe();

	/// Grabs a screenshot of the given device.
	/// Can throw exceptions on error.
	wxBitmap GetScreenshot(const Barry::ProbeResult &device) const;

	//
	// overrides
	//
	virtual bool OnInit();
	virtual int OnExit();
};

DECLARE_APP(BarryDesktopApp)

class UsbScanSplash
{
	std::auto_ptr<wxSplashScreen> m_splash;
public:
	UsbScanSplash()
	{
		wxImage scanpng(GetImageFilename(_T("scanning.png")));
		wxBitmap scanning(scanpng);
		std::auto_ptr<wxSplashScreen> splash( new wxSplashScreen(
			scanning, wxSPLASH_CENTRE_ON_SCREEN, 0,
			NULL, -1, wxDefaultPosition, wxDefaultSize,
			wxSIMPLE_BORDER) );
		splash->Show(true);
		wxGetApp().Yield();
		wxGetApp().Yield();
	}
};

//////////////////////////////////////////////////////////////////////////////
// Utility functions

/// Returns full path and filename for given filename.
/// 'filename' should have no directory component, as the
/// directory will be prepended and returned.
wxString GetImageFilename(const wxString &filename)
{
	// try the official install directory first
	wxString file = _T(BARRYDESKTOP_IMAGEDIR);
	file += filename;
	if( wxFileExists(file) )
		return file;

	// oops, assume we're running from the build directory,
	// and use the images dir
	file = wxPathOnly(wxGetApp().argv[0]);
	file += _T("/../images/");
	file += filename;
	return file;
}

wxString GetButtonFilename(int id, int state)
{
	return GetImageFilename(
		wxString(ButtonNames[id - MainMenu_FirstButton]) + 
		StateNames[state]
		);
}

//////////////////////////////////////////////////////////////////////////////
// ClickableImage

ClickableImage::ClickableImage(wxWindow *parent,
				const wxBitmap &image,
				int ID,
				int x, int y,
				bool event_on_up,
				const wxCursor &hover)
	: m_parent(parent)
	, m_id(ID)
	, m_image(image)
	, m_x(x)
	, m_y(y)
	, m_focus(false)
	, m_event_on_up(event_on_up)
	, m_hover_cursor(hover)
{
}

bool ClickableImage::CalculateHit(int x, int y)
{
	return  ( x >= m_x && x < (m_x + m_image.GetWidth()) )
		&&
		( y >= m_y && y < (m_y + m_image.GetHeight()) );
}

void ClickableImage::Draw(wxDC &dc)
{
	dc.DrawBitmap(m_image, m_x, m_y, true);
}

void ClickableImage::HandleMotion(wxDC &dc, int x, int y)
{
	bool focus = CalculateHit(x, y);

	if( focus && !m_focus ) {
		// newly in focus
		m_parent->SetCursor(m_hover_cursor);
	}
	else if( m_focus && !focus ) {
		// not in focus anymore
		m_parent->SetCursor(wxNullCursor);
	}

	// remember state
	m_focus = focus;
}

void ClickableImage::HandleDown(wxDC &dc, int x, int y)
{
	if( !m_event_on_up ) {
		m_focus = CalculateHit(x, y);

		if( m_focus ) {
			// replace the cursor
			m_parent->SetCursor(wxNullCursor);
			m_focus = false;

			// send the event
			wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED,m_id);
			m_parent->GetEventHandler()->ProcessEvent(event);
		}
	}
}

void ClickableImage::HandleUp(wxDC &dc, int x, int y)
{
	if( m_event_on_up ) {
		m_focus = CalculateHit(x, y);

		if( m_focus ) {
			// replace the cursor
			m_parent->SetCursor(wxNullCursor);
			m_focus = false;

			// send the event
			wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED,m_id);
			m_parent->GetEventHandler()->ProcessEvent(event);
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// PNGButton

PNGButton::PNGButton(wxWindow *parent, int ID, int x, int y)
	: m_parent(parent)
	, m_id(ID)
	, m_x(x)
	, m_y(y)
	, m_state(0)
{
	// normal[0]
	m_bitmaps[BUTTON_STATE_NORMAL] = LoadButtonBitmap(BUTTON_STATE_NORMAL);

	// focus[1]
	m_bitmaps[BUTTON_STATE_FOCUS] = LoadButtonBitmap(BUTTON_STATE_FOCUS);

	// pushed[2]
	m_bitmaps[BUTTON_STATE_PUSHED] = LoadButtonBitmap(BUTTON_STATE_PUSHED);
}

wxBitmap PNGButton::LoadButtonBitmap(int state)
{
	wxString file = GetButtonFilename(m_id, state);
	wxImage image(file);
	wxBitmap bmp(image);
	if( !image.IsOk() || !bmp.IsOk() ) {
		wxGetApp().Yield();
		throw std::runtime_error("Cannot load button bitmap.");
	}
	return bmp;
}

void PNGButton::Init(wxDC &dc)
{
	int width = m_bitmaps[BUTTON_STATE_NORMAL].GetWidth();
	int height = m_bitmaps[BUTTON_STATE_NORMAL].GetHeight();

	m_background = wxBitmap(width, height);

	wxMemoryDC grab_dc;
	grab_dc.SelectObject(m_background);
	grab_dc.Blit(0, 0, width, height, &dc, m_x, m_y, wxCOPY, false);
}

void PNGButton::Draw(wxDC &dc)
{
	dc.DrawBitmap(m_background, m_x, m_y, false);
	dc.DrawBitmap(m_bitmaps[m_state], m_x, m_y);
}

void PNGButton::Normal(wxDC &dc)
{
	m_state = BUTTON_STATE_NORMAL;
	Draw(dc);
}

void PNGButton::Focus(wxDC &dc)
{
	m_state = BUTTON_STATE_FOCUS;
	Draw(dc);
}

void PNGButton::Push(wxDC &dc)
{
	m_state = BUTTON_STATE_PUSHED;
	Draw(dc);
}

void PNGButton::Click(wxDC &dc)
{
	if( IsPushed() ) {
		// return to normal
		m_state = BUTTON_STATE_NORMAL;
		Draw(dc);

		// send the event
		wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, m_id);
		m_parent->GetEventHandler()->ProcessEvent(event);
	}
}

//////////////////////////////////////////////////////////////////////////////
// BaseButtons

BaseButtons::BaseButtons(wxWindow *parent)
	: m_current(0)
{
	// first, discover the size of the average button
	wxString file = GetButtonFilename(MainMenu_BackupAndRestore,
					BUTTON_STATE_NORMAL);
	wxImage sizer(file);
	m_buttonWidth = sizer.GetWidth();
	m_buttonHeight = sizer.GetHeight();

	for( int i = MainMenu_FirstButton; i <= MainMenu_LastButton; i++ ) {
		int row = (i - MainMenu_FirstButton) / 3;
		int col = (i - MainMenu_FirstButton) % 3;
		int y_offset = MAIN_HEADER_OFFSET; // skip the header

		PNGButton *button = new PNGButton(parent, i,
			col * m_buttonWidth,
			row * m_buttonHeight + y_offset);

		m_buttons.push_back(button);
	}
}

BaseButtons::~BaseButtons()
{
	vector<PNGButton*>::iterator b = m_buttons.begin();
	for( ; b != m_buttons.end(); ++b ) {
		delete *b;
	}
	m_buttons.clear();
}

PNGButton* BaseButtons::CalculateHit(int x, int y)
{
	int col = x / m_buttonWidth;
	if( x < 0 || col < 0 || col > 2 )
		return 0;

	int y_offset = MAIN_HEADER_OFFSET;	// graphic header size

	int row = (y - y_offset) / m_buttonHeight;
	if( y < y_offset || row < 0 || row > 2 )
		return 0;

	unsigned int index = col + row * 3;
	if( index >= m_buttons.size() )
		return 0;

	return m_buttons[index];
}

void BaseButtons::InitAll(wxDC &dc)
{
	vector<PNGButton*>::iterator b = m_buttons.begin();
	for( ; b != m_buttons.end(); ++b ) {
		(*b)->Init(dc);
	}
}

void BaseButtons::DrawAll(wxDC &dc)
{
	vector<PNGButton*>::iterator b = m_buttons.begin();
	for( ; b != m_buttons.end(); ++b ) {
		(*b)->Draw(dc);
	}
}

void BaseButtons::HandleMotion(wxDC &dc, int x, int y)
{
	PNGButton *hit = CalculateHit(x, y);
	if( hit != m_current ) {
		// clean up old hit
		if( m_current ) {
			m_current->Normal(dc);
		}

		m_current = hit;

		// draw the new state
		if( m_current )
			m_current->Focus(dc);
	}
}

void BaseButtons::HandleDown(wxDC &dc, int x, int y)
{
	HandleMotion(dc, x, y);
	if( m_current ) {
		m_current->Push(dc);
	}
}

void BaseButtons::HandleUp(wxDC &dc, int x, int y)
{
	HandleMotion(dc, x, y);
	if( m_current && m_current->IsPushed() ) {
		m_current->Click(dc);
	}
}

//////////////////////////////////////////////////////////////////////////////
// MainMenuMode

MainMenuMode::MainMenuMode(wxWindow *parent)
	: m_basebuttons( new BaseButtons(parent) )
{
}

void MainMenuMode::UpdateScreenshot(const Barry::Pin &pin)
{
	// clear existing screenshot
	m_screenshot = wxBitmap();

	// fetch the new device's screenshot
	try {
		if( pin.valid() ) {
			int index = Barry::Probe::FindActive(wxGetApp().GetResults(), pin);
			if( index != -1 ) {
				wxBusyCursor wait;
				m_screenshot = wxGetApp().GetScreenshot(wxGetApp().GetResults()[index]);
			}
		}
	}
	catch( Barry::Error & ) {
		// don't worry if we can't get a screenshot... not all
		// devices support it
	}
}

void MainMenuMode::OnPaint(wxDC &dc)
{
static bool init = false;

	// paint the buttons
	if( !init ) {
		m_basebuttons->InitAll(dc);
		init = true;
	}
	m_basebuttons->DrawAll(dc);

	// paint the screenshot if available
	if( m_screenshot.IsOk() ) {
		dc.DrawBitmap(m_screenshot, 410, 290);
	}
}

void MainMenuMode::OnMouseMotion(wxDC &dc, int x, int y)
{
	m_basebuttons->HandleMotion(dc, x, y);
}

void MainMenuMode::OnLeftDown(wxDC &dc, int x, int y)
{
	m_basebuttons->HandleDown(dc, x, y);
}

void MainMenuMode::OnLeftUp(wxDC &dc, int x, int y)
{
	m_basebuttons->HandleUp(dc, x, y);
}

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

#define BORDER_WIDTH		10	// border to edge of client size
#define BORDER_HEIGHT		10
#define GROUP_BORDER_WIDTH	8	// border between group & list
#define GROUP_BORDER_TOP	20
#define GROUP_BORDER_BOTTOM	8
#define STATUS_HEIGHT		140	// space above list for status info
					// does not include MAIN_HEADER_OFFSET

	// Sync Now button
	m_sync_now_button.reset( new wxButton(parent, SyncMode_SyncNowButton,
		_T("Sync Now"), wxPoint(0, 0), wxDefaultSize) );
	wxSize button_size = m_sync_now_button->GetSize();
	int button_x = client_size.GetWidth() -
			button_size.GetWidth() -
			BORDER_WIDTH;
	m_sync_now_button->Move(button_x, MAIN_HEADER_OFFSET + 10);

	// Device ListCtrl
	int device_y = STATUS_HEIGHT + MAIN_HEADER_OFFSET;
	wxPoint group_point(BORDER_WIDTH, device_y);
	wxSize group_size(client_size.GetWidth() - BORDER_WIDTH*2,
			client_size.GetHeight() - device_y -
			MAIN_HEADER_OFFSET - BORDER_HEIGHT);

	m_box.reset( new wxStaticBox(parent, -1, _T("Device List"),
		group_point, group_size) );

	wxPoint list_point(group_point.x + GROUP_BORDER_WIDTH,
		group_point.y + GROUP_BORDER_TOP);
	wxSize list_size(group_size.GetWidth() - GROUP_BORDER_WIDTH*2,
		group_size.GetHeight() - GROUP_BORDER_TOP - GROUP_BORDER_BOTTOM);

	m_device_list.reset( new wxListCtrl(parent, SyncMode_DeviceList,
		list_point, list_size, wxLC_REPORT /*| wxLC_VRULES*/) );
	m_device_list->InsertColumn(0, _T("PIN"),
		wxLIST_FORMAT_LEFT, list_size.GetWidth() * 0.16);
	m_device_list->InsertColumn(1, _T("Name"),
		wxLIST_FORMAT_LEFT, list_size.GetWidth() * 0.35);
	m_device_list->InsertColumn(2, _T("Connected"),
		wxLIST_FORMAT_CENTRE, list_size.GetWidth() * 0.16);
	m_device_list->InsertColumn(3, _T("Configured"),
		wxLIST_FORMAT_CENTRE, list_size.GetWidth() * 0.16);
	m_device_list->InsertColumn(4, _T("Engine"),
		wxLIST_FORMAT_CENTRE, list_size.GetWidth() * 0.17);

	// Static text
	m_label.reset( new wxStaticText(parent, -1, _T("Static Text"),
		wxPoint(15, 100)) );

	FillDeviceList();

	// connect ourselves to the parent's event handling chain
	// do this last, so that we are guaranteed our destructor
	// will run, in case of exceptions
	m_parent->PushEventHandler(this);
}

SyncMode::~SyncMode()
{
	m_parent->PopEventHandler();
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

		text = i->IsConfigured() ? _T("Yes") : _T("No");
		m_device_list->SetItem(item, 3, text);

		if( i->GetEngine() )
			text = wxString(i->GetEngine()->GetVersion(), wxConvUTF8);
		else
			text = _T("");
		m_device_list->SetItem(item, 4, text);
	}
}

void SyncMode::OnSyncNow(wxCommandEvent &event)
{
	wxMessageBox(_T("Sync Now!"));
}

void SyncMode::OnConfigureDevice(wxListEvent &event)
{
//	wxString msg;
//	msg.Printf(_T("OnConfigureDevice(%ld)"), event.GetIndex());
//	wxMessageBox(msg);

	GroupCfgDlg dlg(m_parent, (*m_device_set)[event.GetIndex()],
		(*m_device_set)[event.GetIndex()].GetEngine(),
		wxGetApp().GetOpenSync());
	dlg.ShowModal();
}

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
	// create the button
	int x = 10;
	int y = m_height - (MAIN_HEADER_OFFSET - 5);

	m_back_button.reset( new wxButton(this, MainMenu_BackButton,
		_T("Main Menu"), wxPoint(x, y), wxDefaultSize,
		wxBU_EXACTFIT) );

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
	info.AddDeveloper(_T("Chris Frey <cdfrey@foursquare.net>"));
	info.AddDeveloper(_T("See AUTHORS file for detailed"));
	info.AddDeveloper(_T("contribution information."));

	info.AddArtist(_T("Chris Frey - GUI interface"));
	info.AddArtist(_T("Martin Owens - Barry logo"));

	wxAboutBox(info);
}

void BaseFrame::OnExit(wxCommandEvent &event)
{
	Close(true);
}

//////////////////////////////////////////////////////////////////////////////
// BarryDesktopApp

BarryDesktopApp::BarryDesktopApp()
	: m_global_config("BarryDesktop")
	, m_set( new OpenSync::APISet )
{
}

void BarryDesktopApp::Probe()
{
	// start fresh
	m_results.clear();

	try {
		// This can throw Usb::Error exceptions
		Barry::Probe probe;
		m_results = probe.GetResults();
	}
	catch( Usb::Error &e ) {
		wxString msg = _T("A serious error occurred while probing the USB subsystem for BlackBerry devices: ");
		msg += wxString(e.what(), wxConvUTF8);
		wxMessageBox(msg, _T("USB Error"), wxOK | wxICON_ERROR);
	}
}

wxBitmap BarryDesktopApp::GetScreenshot(const Barry::ProbeResult &device) const
{
	// FIXME - will need to eventually move the controller object
	// into the main app, I think, and maybe the modes too, so
	// that multiple menu commands can work simultaneously

	Barry::Controller con(device);
	Barry::Mode::JavaLoader javaloader(con);

	javaloader.Open();
	javaloader.StartStream();

	Barry::JLScreenInfo info;
	Barry::Data image;
	javaloader.GetScreenshot(info, image);

	// Convert to BMP format
	Barry::Data bitmap(-1, GetTotalBitmapSize(info));
	Barry::ScreenshotToBitmap(info, image, bitmap);

	// Load as wxImage (sigh)
	wxMemoryInputStream stream(bitmap.GetData(), bitmap.GetSize());
	wxImage bmp(stream, wxBITMAP_TYPE_BMP);
	bmp.Rescale(180, 100, wxIMAGE_QUALITY_HIGH);
	return wxBitmap(bmp);
}

void BarryDesktopApp::ShowMissingOpenSyncMessage()
{
	wxMessageBox(_T("No OpenSync libraries were found. Sync will be unavailable until you install OpenSync version 0.22 or version 0.4x on your system, along with the needed plugins."), _T("OpenSync Not Found"), wxOK | wxICON_INFORMATION);
}

bool BarryDesktopApp::OnInit()
{
	// Add a PNG handler for loading buttons and backgrounds
	wxImage::AddHandler( new wxPNGHandler );

	std::auto_ptr<UsbScanSplash> splash( new UsbScanSplash );

	// Initialize Barry and USB
	Barry::Init(m_global_config.VerboseLogging());

	// Scan bus at the beginning so we know what devices we've got
	Probe();

	// Search for available OpenSync libraries
	if( m_set->OpenAvailable() == 0 ) {
		ShowMissingOpenSyncMessage();
	}

	// Create the main frame window where all the action happens
	wxImage back(GetImageFilename(_T("background.png")));
	if( !back.IsOk() ) {
		Yield();
		return false;
	}
	BaseFrame *frame = new BaseFrame(back);

	// Clean up the splash screen, and init the main frame
	splash.reset();
	SetTopWindow(frame);
	frame->Show(true);

	return true;
}

int BarryDesktopApp::OnExit()
{
	try {
		m_global_config.Save();
	}
	catch( std::exception &e ) {
		cerr << "Exception caught while saving config: "
			<< e.what() << endl;
	}

	return 0;
}

// This takes care of main() and wxGetApp() for us.
IMPLEMENT_APP(BarryDesktopApp)

