///
/// \file	Mode_MainMenu.cc
///		Mode derived class for the main menu buttons
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

#include "Mode_MainMenu.h"
#include "BaseButtons.h"
#include "barrydesktop.h"
#include <barry/barry.h>
#include "i18n.h"

//////////////////////////////////////////////////////////////////////////////
// MainMenuMode

MainMenuMode::MainMenuMode(wxWindow *parent)
	: m_basebuttons( new BaseButtons(parent) )
{
}

MainMenuMode::~MainMenuMode()
{
}

void MainMenuMode::UpdateScreenshot(const Barry::Pin &pin)
{
	// clear existing screenshot
	m_screenshot = wxBitmap();

	// fetch the new device's screenshot
	try {
		if( pin.Valid() ) {
			int index = Barry::Probe::FindActive(wxGetApp().GetResults(), pin);
			if( index != -1 ) {
				wxBusyCursor wait;
				m_screenshot = wxGetApp().GetScreenshot(wxGetApp().GetResults()[index]);
			}
		}
	}
	catch( Barry::Error &be ) {
		// don't worry if we can't get a screenshot... not all
		// devices support it
		barryverbose(_C("Ignorable screenshot exception: ") << be.what());
	}
}

void MainMenuMode::OnPaint(wxDC &dc)
{
static bool init = false;

	// paint the buttons
	if( !init ) {
		m_basebuttons->InitAll(dc);
		init = true;
	}
	m_basebuttons->DrawAll(dc);

	// paint the screenshot if available
	if( m_screenshot.IsOk() ) {
		dc.DrawBitmap(m_screenshot, 410, 290);
	}
}

void MainMenuMode::OnMouseMotion(wxDC &dc, int x, int y)
{
	m_basebuttons->HandleMotion(dc, x, y);
}

void MainMenuMode::OnLeftDown(wxDC &dc, int x, int y)
{
	m_basebuttons->HandleDown(dc, x, y);
}

void MainMenuMode::OnLeftUp(wxDC &dc, int x, int y)
{
	m_basebuttons->HandleUp(dc, x, y);
}

