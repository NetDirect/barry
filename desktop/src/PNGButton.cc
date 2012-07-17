///
/// \file	PNGButton.cc
///		Class for turning a set of PNG images into buttons
///

/*
    Copyright (C) 2009-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "PNGButton.h"
#include "barrydesktop.h"
#include "i18n.h"

//////////////////////////////////////////////////////////////////////////////
// PNGButton

PNGButton::PNGButton(wxWindow *parent, int ID, int x, int y, bool enabled)
	: m_parent(parent)
	, m_id(ID)
	, m_x(x)
	, m_y(y)
	, m_state(0)
	, m_enabled(enabled)
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
		throw std::runtime_error(_C("Cannot load button bitmap."));
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
	if( !m_enabled )
		return;

	m_state = BUTTON_STATE_NORMAL;
	Draw(dc);
}

void PNGButton::Focus(wxDC &dc)
{
	if( !m_enabled )
		return;

	m_state = BUTTON_STATE_FOCUS;
	Draw(dc);
}

void PNGButton::Push(wxDC &dc)
{
	if( !m_enabled )
		return;

	m_state = BUTTON_STATE_PUSHED;
	Draw(dc);
}

void PNGButton::Click(wxDC &dc)
{
	if( !m_enabled )
		return;

	if( IsPushed() ) {
		// return to normal
		m_state = BUTTON_STATE_NORMAL;
		Draw(dc);

		// send the event
		wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, m_id);
		m_parent->GetEventHandler()->ProcessEvent(event);
	}
}

