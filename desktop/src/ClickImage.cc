///
/// \file	ClickImage.cc
///		Clickable image class
///

/*
    Copyright (C) 2009-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "ClickImage.h"

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

