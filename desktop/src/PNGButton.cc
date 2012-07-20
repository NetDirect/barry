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
#include "wxi18n.h"

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

	// and the label text, translated
	m_label = GetButtonLabel(m_id);
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

//
// Due to limitations in DC transparency support, and for speed reasons,
// we use this Init() opportunity to create a brand new bitmap for each
// button, which includes the background and the transparency bitmap drawing,
// and the button text.
//
void PNGButton::Init(wxDC &dc)
{
	// grab the existing background first
	int width = m_bitmaps[BUTTON_STATE_NORMAL].GetWidth();
	int height = m_bitmaps[BUTTON_STATE_NORMAL].GetHeight();

	m_background = wxBitmap(width, height);

	// copy it over, no transparency
	{
		wxMemoryDC grab_dc;
		grab_dc.SelectObject(m_background);
		grab_dc.Blit(0, 0, width, height, &dc, m_x, m_y, wxCOPY, false);
	}

	// this font may be modified by DrawButtonLabelDC... keep it
	// outside the loop so the same size font will be used for all
	// buttons
	int pointsize = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)
		.GetPointSize();
	wxFont font(pointsize + 2, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
		wxFONTWEIGHT_BOLD);

	// for each button, draw it with background + transparency + label
	for( int i = 0; i < 3; i++ ) {
		wxBitmap final = wxBitmap(width, height);

		{
			wxMemoryDC dc;
			dc.SelectObject(final);

			// draw the background, no transparency
			dc.DrawBitmap(m_background, 0, 0, false);

			// draw the button, with transparency
			dc.DrawBitmap(m_bitmaps[i], 0, 0);

			// draw the text
			DrawButtonLabelDC(dc, m_bitmaps[i],
				m_label, font, *wxBLACK, 80, 12, -15, -15);
		}

		// copy final button bitmap into m_bitmaps array for use
		m_bitmaps[i] = final;
	}
}

void PNGButton::Draw(wxDC &dc)
{
	// just splat the final button onto the screen at the exepcted
	// coordinates
	dc.DrawBitmap(m_bitmaps[m_state], m_x, m_y, false);
}

/// This is only used if the button is moved, and the background needs to
/// be restored.  Currently only theoretical.
void PNGButton::Erase(wxDC &dc)
{
	dc.DrawBitmap(m_background, m_x, m_y, false);
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

