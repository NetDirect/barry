///
/// \file	util.cc
///		Utility functions specific to Barry Desktop
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

#include "util.h"
#include "barrydesktop.h"
#include "windowids.h"

const wxChar *ButtonNames[] = {
	_T("backuprestore"),
	_T("sync"),
	_T("modem"),
	_T("apploader"),
	_T("deviceswitch"),
	_T("browsedatabases"),
	_T("media"),
	_T("misc"),
	0
	};

const wxChar *StateNames[] = {
	_T("-normal.png"),
	_T("-focus.png"),
	_T("-pushed.png"),
	0
	};

//////////////////////////////////////////////////////////////////////////////
// Utility functions

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
	return file;
}

wxString GetButtonFilename(int id, int state)
{
	return GetImageFilename(
		wxString(ButtonNames[id - MainMenu_FirstButton]) + 
		StateNames[state]
		);
}
