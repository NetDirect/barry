///
/// \file	Mode_MainMenu.h
///		Mode derived class for the main menu buttons
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

#ifndef __BARRYDESKTOP_MODE_MAINMENU_H__
#define __BARRYDESKTOP_MODE_MAINMENU_H__

#include <wx/wx.h>
#include <memory>
#include "Mode.h"
#include "wxi18n.h"

class BaseButtons;
namespace Barry {
	class Pin;
}

class MainMenuMode : public Mode
{
	std::auto_ptr<BaseButtons> m_basebuttons;
	wxBitmap m_screenshot;

public:
	MainMenuMode(wxWindow *parent);
	~MainMenuMode();

	void UpdateScreenshot(const Barry::Pin &pin);

	// events (called from BaseFrame)
	wxString GetTitleText() const
	{
		return _W("Barry Desktop Control Panel");
	}

	void OnPaint(wxDC &dc);
	void OnMouseMotion(wxDC &dc, int x, int y);
	void OnLeftDown(wxDC &dc, int x, int y);
	void OnLeftUp(wxDC &dc, int x, int y);
};

#endif

