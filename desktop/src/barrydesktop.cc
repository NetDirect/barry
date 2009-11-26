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

/*
const char *ButtonNames[] = {
	_T("backuprestore"),
	_T("sync"),
	_T("modem"),
	_T("apploader"),
	_T("deviceswitch"),
	_T("browsedatabases"),
	_T("MediaManagement"),
	_T("Misc"),
	0
	};
*/
const wxChar *ButtonNames[] = {
	_T("backuprestore"),
	_T("backuprestore"),
	_T("backuprestore"),
	_T("backuprestore"),
	_T("backuprestore"),
	_T("backuprestore"),
	_T("backuprestore"),
	_T("backuprestore"),
	0
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
	wxBitmap LoadButtonBitmap(const wxString &state);

public:
	PNGButton(wxWindow *parent, int ID, int x, int y);

	bool IsPushed() const { return m_state == 2; }

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
	BaseFrame();

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
	m_bitmaps[0] = LoadButtonBitmap(_T("-normal.png"));

	// focus[1]
	m_bitmaps[1] = LoadButtonBitmap(_T("-focus.png"));

	// pushed[2]
	m_bitmaps[2] = LoadButtonBitmap(_T("-pushed.png"));
}

wxBitmap PNGButton::LoadButtonBitmap(const wxString &state)
{
	wxString file = _T("../images/");
	file += ButtonNames[m_id - MainMenu_FirstButton];
	file += state;

	wxImage image(file);
	return wxBitmap(image);
}

void PNGButton::Init(wxDC &dc)
{
	int width = m_bitmaps[0].GetWidth();
	int height = m_bitmaps[0].GetHeight();

	m_background = wxBitmap(width, height);

	wxMemoryDC grab_dc;
	grab_dc.SelectObject(m_background);
	grab_dc.Blit(0, 0, m_bitmaps[0].GetWidth(), m_bitmaps[0].GetHeight(),
		&dc, m_x, m_y, wxCOPY, false);
}

void PNGButton::Draw(wxDC &dc)
{
	dc.DrawBitmap(m_background, m_x, m_y, false);
	dc.DrawBitmap(m_bitmaps[m_state], m_x, m_y);
}

void PNGButton::Normal(wxDC &dc)
{
	m_state = 0;
	Draw(dc);
}

void PNGButton::Focus(wxDC &dc)
{
	m_state = 1;
	Draw(dc);
}

void PNGButton::Push(wxDC &dc)
{
	m_state = 2;
	Draw(dc);
}

void PNGButton::Click(wxDC &dc)
{
	if( IsPushed() ) {
		// return to normal
		m_state = 0;
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
	for( int i = MainMenu_FirstButton; i <= MainMenu_LastButton; i++ ) {
		int row = (i - MainMenu_FirstButton) / 3;
		int col = (i - MainMenu_FirstButton) % 3;
		int y_offset = 40; // skip the header

		PNGButton *button = new PNGButton(parent, i,
			col * 200, row * 120 + y_offset);

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
	int col = x / 200;
	if( col < 0 || col > 2 )
		return 0;

	int y_offset = 40;	// graphic header size

	int row = (y - y_offset) / 120;
	if( row < 0 || row > 2 )
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

BaseFrame::BaseFrame()
	: wxFrame(NULL, wxID_ANY, _T("Barry Desktop"),
		wxPoint(50, 50), wxSize(600, 400),
		wxMINIMIZE_BOX | wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU |
		wxCLIP_CHILDREN)
{
	wxImage back(_T("../images/background.png"));
	m_background.reset( new wxBitmap(back) );

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
	BaseFrame *frame = new BaseFrame;
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

