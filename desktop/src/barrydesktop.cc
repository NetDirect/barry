///
/// \file	barrydesktop.cc
///		Program entry point for the desktop GUI
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

// include icons and logos
#include "../images/barry_logo_icon.xpm"
#include "../images/logo_NetDirect.xpm"

using namespace std;

#define MAIN_HEADER_OFFSET 40

//////////////////////////////////////////////////////////////////////////////
// IDs for controls and menu items (no menus in this app yet)
enum {
	SysMenu_Exit = wxID_EXIT,
	SysMenu_About = wxID_ABOUT,

	MainMenu_FirstButton = wxID_HIGHEST,

	MainMenu_BackupAndRestore = MainMenu_FirstButton,
	MainMenu_Sync,
	MainMenu_Modem,
	MainMenu_AppLoader,
	MainMenu_DeviceSwitch,
	MainMenu_BrowseDatabases,
	MainMenu_MediaManagement,
	MainMenu_Misc,

	MainMenu_LastButton = MainMenu_Misc,

	// Clickable, "hot" images that do something
	HotImage_BarryLogo,
	HotImage_NetDirectLogo
};

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

wxString GetButtonFilename(int id, int state);

wxString GetButtonFilename(int id, int state)
{
	wxString file = _T("../images/");
	file += ButtonNames[id - MainMenu_FirstButton];
	file += StateNames[state];
	return file;
}

class ClickableImage
{
	wxWindow *m_parent;
	int m_id;
	wxBitmap m_image;
	int m_x, m_y;
	bool m_focus;
	wxCursor m_hover_cursor;

protected:
	bool CalculateHit(int x, int y);

public:
	ClickableImage(wxWindow *parent, const wxBitmap &image,
		int ID, int x, int y,
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

class BaseFrame : public wxFrame
{
private:
	DECLARE_EVENT_TABLE()

private:
	std::auto_ptr<wxBitmap> m_background, m_button;
	std::auto_ptr<BaseButtons> m_basebuttons;
	std::auto_ptr<ClickableImage> m_barry_logo, m_netdirect_logo;
	std::auto_ptr<wxMenu> m_sysmenu;
	int m_width, m_height;

public:
	BaseFrame(const wxImage &background);

	void OnPaint(wxPaintEvent &event);
	void OnMouseMotion(wxMouseEvent &event);
	void OnLeftDown(wxMouseEvent &event);
	void OnLeftUp(wxMouseEvent &event);
	void OnBackupRestore(wxCommandEvent &event);
	void OnBarryLogoClicked(wxCommandEvent &event);
	void OnNetDirectLogoClicked(wxCommandEvent &event);

	// sys menu (triggered by the Barry logo)
	void OnAbout(wxCommandEvent &event);
	void OnExit(wxCommandEvent &event);
};

BEGIN_EVENT_TABLE(BaseFrame, wxFrame)
	EVT_PAINT	(BaseFrame::OnPaint)
	EVT_MOTION	(BaseFrame::OnMouseMotion)
	EVT_LEFT_DOWN	(BaseFrame::OnLeftDown)
	EVT_LEFT_UP	(BaseFrame::OnLeftUp)
	EVT_BUTTON	(MainMenu_BackupAndRestore, BaseFrame::OnBackupRestore)
	EVT_BUTTON	(HotImage_BarryLogo, BaseFrame::OnBarryLogoClicked)
	EVT_BUTTON	(HotImage_NetDirectLogo, BaseFrame::OnNetDirectLogoClicked)
	EVT_MENU	(SysMenu_About, BaseFrame::OnAbout)
	EVT_MENU	(SysMenu_Exit, BaseFrame::OnExit)
END_EVENT_TABLE()

class BarryDesktopApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int OnExit();
};

//////////////////////////////////////////////////////////////////////////////
// ClickableImage

ClickableImage::ClickableImage(wxWindow *parent,
				const wxBitmap &image,
				int ID,
				int x, int y,
				const wxCursor &hover)
	: m_parent(parent)
	, m_id(ID)
	, m_image(image)
	, m_x(x)
	, m_y(y)
	, m_focus(false)
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
}

void ClickableImage::HandleUp(wxDC &dc, int x, int y)
{
	m_focus = CalculateHit(x, y);

	if( m_focus ) {
		// replace the cursor
		m_parent->SetCursor(wxNullCursor);
		m_focus = false;

		// send the event
		wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, m_id);
		m_parent->GetEventHandler()->ProcessEvent(event);
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
	return wxBitmap(image);
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
// BaseFrame

BaseFrame::BaseFrame(const wxImage &background)
	: wxFrame(NULL, wxID_ANY, _T("Barry Desktop"),
		wxPoint(50, 50),
		wxSize(background.GetWidth(), background.GetHeight()),
		wxMINIMIZE_BOX | wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU |
		wxCLIP_CHILDREN)
	, m_width(background.GetWidth())
	, m_height(background.GetHeight())
{
	m_background.reset( new wxBitmap(background) );
	m_basebuttons.reset( new BaseButtons(this) );
	m_barry_logo.reset( new ClickableImage(this,
		wxBitmap(barry_logo_icon_xpm), HotImage_BarryLogo, 4, 4) );
	wxBitmap nd_logo(logo_NetDirect_xpm);
	m_netdirect_logo.reset( new ClickableImage(this,
		nd_logo, HotImage_NetDirectLogo,
		m_width - 3 - nd_logo.GetWidth(),
		(MAIN_HEADER_OFFSET - nd_logo.GetHeight()) / 2,
		wxNullCursor));

	// Create the Barry Logo popup system menu
	m_sysmenu.reset( new wxMenu );
	m_sysmenu->Append(SysMenu_About, _T("&About..."));
	m_sysmenu->AppendSeparator();
	m_sysmenu->Append(wxID_EXIT, _T("E&xit"));
}

void BaseFrame::OnPaint(wxPaintEvent &event)
{
static bool init = false;

	// paint the background image
	wxPaintDC dc(this);
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
	wxString header = _T("Barry Desktop");
	dc.GetTextExtent(header, &width, &height, &descent);
	int x = (m_width - width) / 2;
	int y = (MAIN_HEADER_OFFSET - height) / 2;
	dc.DrawText(header, x, y);

	// paint the buttons
	if( !init ) {
		m_basebuttons->InitAll(dc);
		init = true;
	}
	m_basebuttons->DrawAll(dc);
}

void BaseFrame::OnMouseMotion(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_basebuttons->HandleMotion(dc, event.m_x, event.m_y);
	m_barry_logo->HandleMotion(dc, event.m_x, event.m_y);
	m_netdirect_logo->HandleMotion(dc, event.m_x, event.m_y);
}

void BaseFrame::OnLeftDown(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_basebuttons->HandleDown(dc, event.m_x, event.m_y);
	m_barry_logo->HandleDown(dc, event.m_x, event.m_y);
	m_netdirect_logo->HandleDown(dc, event.m_x, event.m_y);
	event.Skip();
}

void BaseFrame::OnLeftUp(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_basebuttons->HandleUp(dc, event.m_x, event.m_y);
	m_barry_logo->HandleUp(dc, event.m_x, event.m_y);
	m_netdirect_logo->HandleUp(dc, event.m_x, event.m_y);
}

void BaseFrame::OnBackupRestore(wxCommandEvent &event)
{
	wxMessageBox(_T("OnBackupRestore"));
}

void BaseFrame::OnBarryLogoClicked(wxCommandEvent &event)
{
	PopupMenu(m_sysmenu.get(), 4, 20);
}

void BaseFrame::OnNetDirectLogoClicked(wxCommandEvent &event)
{
	// FIXME: fire up a browser to point to the Barry
	// documentation at:
	// http://netdirect.ca/barry
	wxMessageBox(_T("OnNetDirectLogoClicked"));
}

void BaseFrame::OnAbout(wxCommandEvent &event)
{
	wxMessageBox(_T("FIXME: About box needs to be implemented"));
}

void BaseFrame::OnExit(wxCommandEvent &event)
{
	Close(true);
}

//////////////////////////////////////////////////////////////////////////////
// BarryDesktopApp

bool BarryDesktopApp::OnInit()
{
	// Initialize Barry and USB
	Barry::Init(true);

	// Add a PNG handler for loading buttons and backgrounds
	wxImage::AddHandler( new wxPNGHandler );

	// Create the main frame window where all the action happens
	wxImage back(_T("../images/background.png"));
	BaseFrame *frame = new BaseFrame(back);
	SetTopWindow(frame);
	frame->Show(true);

	return true;
}

int BarryDesktopApp::OnExit()
{
	return 0;
}

// This takes care of main() and wxGetApp() for us.
IMPLEMENT_APP(BarryDesktopApp)

