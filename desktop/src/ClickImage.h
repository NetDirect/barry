///
/// \file	ClickImage.h
///		Clickable image class
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

#ifndef __BARRYDESKTOP_CLICKIMAGE_H__
#define __BARRYDESKTOP_CLICKIMAGE_H__

#include <wx/wx.h>

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

#endif

