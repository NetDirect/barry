///
/// \file	Mode.h
///		Mode base class... each main button gets a mode
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

#ifndef __BARRYDESKTOP_MODE_H__
#define __BARRYDESKTOP_MODE_H__

#include <wx/wx.h>

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

#endif

