///
/// \file	util.cc
///		Utility functions specific to Barry Desktop
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

#include "util.h"
#include "barrydesktop.h"
#include "windowids.h"
#include <wx/datectrl.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include "wxi18n.h"

using namespace std;

const wxChar *ButtonNames[] = {
	_T("backuprestore"),
	_T("sync"),
	_T("modem"),
	_T("migratedevice"),
	_T("browsedatabases"),
	_T("apploader"),
	_T("media"),
	_T("misc"),
	0
	};

const wxArrayString& GetButtonLabels()
{
	static wxArrayString m_labels;

	if( m_labels.GetCount() == 0 ) {
		// TRANSLATORS: this is a main screen button label. See
		// util.cc line 200 for more information on its flexibility.
		m_labels.Add( _W("Backup &\nRestore") );
		// TRANSLATORS: this is a main screen button label
		m_labels.Add( _W("Sync") );
		// TRANSLATORS: this is a main screen button label
		m_labels.Add( _W("Modem\nTethering") );
		// TRANSLATORS: this is a main screen button label
		m_labels.Add( _W("Migrate\nDevice") );
		// TRANSLATORS: this is a main screen button label
		m_labels.Add( _W("Browse\nDatabases") );
		// TRANSLATORS: this is a main screen button label
		m_labels.Add( _W("Application\nLoader") );
		// TRANSLATORS: this is a main screen button label
		m_labels.Add( _W("Media") );
		// TRANSLATORS: this is a main screen button label
		m_labels.Add( _W("Misc") );
	}

	return m_labels;
}

bool ButtonEnabled[] = {
	true,	// backuprestore
	true,	// sync
	true,	// modem
	true,	// migratedevice
	true,	// browsedatabases
	false,	// apploader
	false,	// media
	false,	// misc
	false
	};

const wxChar *StateNames[] = {
	_T("-normal.png"),
	_T("-focus.png"),
	_T("-pushed.png"),
	0
	};

//////////////////////////////////////////////////////////////////////////////
// Utility functions

std::string GetBaseFilename(const std::string &filename)
{
	std::string file = BARRYDESKTOP_BASEDATADIR;
	file += filename;
	if( wxFileExists(wxString(file.c_str(), wxConvUTF8)) )
		return file;

	// fall back to the devel tree
	return filename;
}

/// Returns full path and filename for given filename.
/// 'filename' should have no directory component, as the
/// directory will be prepended and returned.
wxString GetImageFilename(const wxString &filename)
{
	// try the official install directory first
	wxString file = _T(BARRYDESKTOP_IMAGEDIR);
	file += filename;
	if( wxFileExists(file) )
		return file;

	// oops, assume we're running from the build directory,
	// and use the images dir
	file = wxPathOnly(wxGetApp().argv[0]);
	file += _T("/../images/");
	file += filename;
	if( wxFileExists(file) )
		return file;

	// hmmm.... maybe we're running from inside the libtool
	// build subdirectories
	file = wxPathOnly(wxGetApp().argv[0]);
	file += _T("/../../images/");
	file += filename;
	return file;
}

wxString GetButtonFilename(int id, int state)
{
	return GetImageFilename(
		wxString(ButtonNames[id - MainMenu_FirstButton]) + 
		StateNames[state]
		);
}

wxString GetButtonLabel(int id)
{
	return GetButtonLabels()[id - MainMenu_FirstButton];
}

bool IsButtonEnabled(int id)
{
	return ButtonEnabled[id - MainMenu_FirstButton];
}

void MakeDateRecent(bool checked, wxDatePickerCtrl *picker)
{
	wxDateTime when = picker->GetValue();
	if( checked && (!when.IsValid() ||
		when < wxDateTime(1, wxDateTime::Jan, 1975, 0, 0, 0)) )
	{
		when = wxDateTime::Now();
		picker->SetValue(when);
	}
}

bool IsParsable(const std::string &dbname)
{
#undef HANDLE_PARSER
#define HANDLE_PARSER(dbn) \
	if( dbname == Barry::dbn::GetDBName() ) return true;

	ALL_KNOWN_PARSER_TYPES

	return false;
}

bool IsBuildable(const std::string &dbname)
{
#undef HANDLE_BUILDER
#define HANDLE_BUILDER(dbn) \
	if( dbname == Barry::dbn::GetDBName() ) return true;

	ALL_KNOWN_BUILDER_TYPES

	return false;
}

namespace {
	struct LabelLine
	{
		wxString m_line;
		int m_width;
		int m_height;

		LabelLine(const wxString &line, const wxDC &dc)
			: m_line(line)
			, m_width(0)
			, m_height(0)
		{
			dc.GetTextExtent(line, &m_width, &m_height);
		}
	};
}

//
// DrawButtonLabel
//
/// Draws the given label text in the specified area of the bitmap,
/// modifying the DC in the process (the bitmap is only used for
/// sizing).  This is intended for use in creating the main Desktop
/// buttons, but can be used on any image.
///
/// The left/top/right/bottom coordinates are relative to the 0,0 of
/// the bitmap itself, and limit the area where the label text will be
/// drawn.  If -1 is used for any of the values, the bitmap edge will
/// be used.  If other negative numbers are used, (edge - value) will
/// be used.
///
/// The label text can contain \n characters to split into multiple
/// lines.  Each line will be centered (left/right) in the coordinate
/// area, and all lines will be centered (top/bottom) in the coordinate
/// area.  A trailing \n character is not required, and not recommended.
///
/// If the coordinate area is too small for the given text and font,
/// the font will be reduced iteratively for a few points and tried again.
/// If still too big, a DrawButtonLabelError exception will be thrown.
/// Note that the font passed in will be modified in this case, in case
/// the new point size needs to be used elsewhere.
///
/// If the label contains an empty string, a DrawButtonLabelError exception will
/// be thrown.  This is to prevent any unlabeled buttons in the system.
///
/// If Font is invalid, DrawButtonLabelError will be thrown.
///
void DrawButtonLabelDC(wxDC &dc, const wxBitmap &bmp, const wxString &label,
	wxFont &font, const wxColour &textfg,
	int left, int top, int right, int bottom)
{
	// calculate the coordinates, and various sanity checks
	if( left == -1 )	left = 0;
	if( top == -1 )		top = 0;
	if( right == -1 )	right = bmp.GetWidth();
	if( bottom == -1 )	bottom = bmp.GetHeight();

	if( right < -1 )	right = bmp.GetWidth() + right;
	if( bottom < -1 )	bottom = bmp.GetHeight() + bottom;

	int width = right - left;
	if( width < 0 ) {
		swap(left, right);
		width = right - left;
	}

	int height = bottom - top;
	if( height < 0 ) {
		swap(top, bottom);
		height = bottom - top;
	}

	if( !font.IsOk() )
		throw DrawButtonLabelError(_C("Unable to create button: font is invalid"));

	// create DC to work with, writing into the bitmap given to us
	dc.SetFont(font);
	dc.SetTextForeground(textfg);
	dc.SetMapMode(wxMM_TEXT);

	// build vector of lines, and calculate totals
	int total_text_height = 0;
	int widest_line = 0;
	std::vector<LabelLine> lines;

	// ... and keep trying if too big
	for( int tries = 0; ; tries++ ) {
		// start fresh
		total_text_height = 0;
		widest_line = 0;
		lines.clear();

		wxStringTokenizer tokens(label, _T("\n"));
		while( tokens.HasMoreTokens() ) {
			wxString token = tokens.GetNextToken();
			token.Trim(true);
			token.Trim(false);

			if( !tokens.HasMoreTokens() && token.size() == 0 ) {
				// we're done here... last line is empty
				break;
			}

			LabelLine line(token, dc);
			lines.push_back( line );
			total_text_height += line.m_height;
			widest_line = max(widest_line, line.m_width);
		}

		// do we have enough room?
		if( total_text_height <= height && widest_line <= width ) {
			// good to go!
			break;
		}

		// only reduce font so much...
		if( tries >= 4 )
			throw DrawButtonLabelError(_C("Unable to create button: text is too big to fit: ") + string(label.utf8_str()));

		// too big, reduce font and try again
		font.SetPointSize( font.GetPointSize() - 1 );
		dc.SetFont(font);
	}

	// empty?
	if( lines.size() == 0 )
		throw DrawButtonLabelError(_C("Unable to create button: label is empty"));

	// calculate starting height
	int y = (height - total_text_height) / 2 + top;

	// draw each line, centering each one horizontally, and
	// incrementing y by the line's height on each pass
	std::vector<LabelLine>::iterator b = lines.begin(), e = lines.end();
	for( ; b != e; ++b ) {
		int x = (width - b->m_width) / 2 + left;
		dc.DrawText(b->m_line, x, y);
		y += b->m_height;
	}
}

