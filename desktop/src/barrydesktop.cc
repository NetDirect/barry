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

using namespace std;

#define MAIN_HEADER_OFFSET 40

//////////////////////////////////////////////////////////////////////////////
// IDs for controls and menu items (no menus in this app yet)
enum {
	MainMenu_FirstButton = wxID_HIGHEST,

	MainMenu_BackupAndRestore = MainMenu_FirstButton,
	MainMenu_Sync,
	MainMenu_Modem,
	MainMenu_AppLoader,
	MainMenu_DeviceSwitch,
	MainMenu_BrowseDatabases,
	MainMenu_MediaManagement,
	MainMenu_Misc,

	MainMenu_LastButton = MainMenu_Misc
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

public:
	BaseFrame(const wxImage &background);

	void OnPaint(wxPaintEvent &event);
	void OnMouseMotion(wxMouseEvent &event);
	void OnLeftDown(wxMouseEvent &event);
	void OnLeftUp(wxMouseEvent &event);
	void OnBackupRestore(wxCommandEvent &event);
};

BEGIN_EVENT_TABLE(BaseFrame, wxFrame)
	EVT_PAINT	(BaseFrame::OnPaint)
	EVT_MOTION	(BaseFrame::OnMouseMotion)
	EVT_LEFT_DOWN	(BaseFrame::OnLeftDown)
	EVT_LEFT_UP	(BaseFrame::OnLeftUp)
	EVT_BUTTON	(MainMenu_BackupAndRestore, BaseFrame::OnBackupRestore)
END_EVENT_TABLE()

class BarryDesktopApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int OnExit();
};

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
{
	m_background.reset( new wxBitmap(background) );
	m_basebuttons.reset( new BaseButtons(this) );
}

void BaseFrame::OnPaint(wxPaintEvent &event)
{
static bool init = false;

	wxPaintDC dc(this);
	dc.DrawBitmap(*m_background, 0, 0);
	if( !init ) {
		m_basebuttons->InitAll(dc);
		init = true;
	}
	m_basebuttons->DrawAll(dc);

	// FIXME - add a logo here, as a small XPM file in the corner
}

void BaseFrame::OnMouseMotion(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_basebuttons->HandleMotion(dc, event.m_x, event.m_y);
}

void BaseFrame::OnLeftDown(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_basebuttons->HandleDown(dc, event.m_x, event.m_y);
	event.Skip();
}

void BaseFrame::OnLeftUp(wxMouseEvent &event)
{
	wxClientDC dc(this);
	m_basebuttons->HandleUp(dc, event.m_x, event.m_y);
}

void BaseFrame::OnBackupRestore(wxCommandEvent &event)
{
	wxMessageBox(_T("OnBackupRestore"));
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

