///
/// \file	BaseButtons.cc
///		Support class for BaseFrame
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

#include "BaseButtons.h"
#include "BaseFrame.h"
#include "PNGButton.h"
#include "windowids.h"
#include "util.h"
#include <wx/wx.h>
#include <vector>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// BaseButtons

BaseButtons::BaseButtons(wxWindow *parent)
	: m_current(0)
{
	// first, discover the size of the average button
	wxString file = GetButtonFilename(MainMenu_BackupAndRestore,
					BUTTON_STATE_NORMAL);
	wxImage sizer(file);
	m_buttonWidth = sizer.GetWidth();
	m_buttonHeight = sizer.GetHeight();

	for( int i = MainMenu_FirstButton; i <= MainMenu_LastButton; i++ ) {
		int row = (i - MainMenu_FirstButton) / 3;
		int col = (i - MainMenu_FirstButton) % 3;
		int y_offset = MAIN_HEADER_OFFSET; // skip the header

		PNGButton *button = new PNGButton(parent, i,
			col * m_buttonWidth,
			row * m_buttonHeight + y_offset,
			IsButtonEnabled(i));

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
	int col = x / m_buttonWidth;
	if( x < 0 || col < 0 || col > 2 )
		return 0;

	int y_offset = MAIN_HEADER_OFFSET;	// graphic header size

	int row = (y - y_offset) / m_buttonHeight;
	if( y < y_offset || row < 0 || row > 2 )
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

