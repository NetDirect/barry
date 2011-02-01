///
/// \file	util.cc
///		Utility functions specific to Barry Desktop
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

bool ButtonEnabled[] = {
	true,	// backuprestore
	true,	// sync
	false,	// modem
	false,	// apploader
	false,	// deviceswitch
	true,	// browsedatabases
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

bool IsButtonEnabled(int id)
{
	return ButtonEnabled[id - MainMenu_FirstButton];
}

bool IsParsable(const std::string &dbname)
{
#undef HANDLE_PARSER
#define HANDLE_PARSER(dbn) \
	if( dbname == Barry::dbn::GetDBName() ) return true;

	ALL_KNOWN_PARSER_TYPES

	return false;
}

