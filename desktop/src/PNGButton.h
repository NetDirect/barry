///
/// \file	PNGButton.h
///		Class for turning a set of PNG images into buttons
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

#ifndef __BARRYDESKTOP_PNGBUTTON_H__
#define __BARRYDESKTOP_PNGBUTTON_H__

#include <wx/wx.h>
#include "util.h"

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

#endif

