///
/// \file	BaseButtons.h
///		Support class for BaseFrame
///

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_BASEBUTTONS_H__
#define __BARRYDESKTOP_BASEBUTTONS_H__

#include <vector>

class PNGButton;
class wxDC;
class wxWindow;

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

#endif

