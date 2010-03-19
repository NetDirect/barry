///
/// \file	util.h
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

#ifndef __BARRYDESKTOP_UTIL_H__
#define __BARRYDESKTOP_UTIL_H__

#include <wx/wx.h>

#define BUTTON_STATE_NORMAL	0
#define BUTTON_STATE_FOCUS	1
#define BUTTON_STATE_PUSHED	2

wxString GetImageFilename(const wxString &filename);
wxString GetButtonFilename(int id, int state);

#endif

