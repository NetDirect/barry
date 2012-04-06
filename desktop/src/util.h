///
/// \file	util.h
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

#ifndef __BARRYDESKTOP_UTIL_H__
#define __BARRYDESKTOP_UTIL_H__

#include <wx/wx.h>
#include <barry/barry.h>

#define BUTTON_STATE_NORMAL	0
#define BUTTON_STATE_FOCUS	1
#define BUTTON_STATE_PUSHED	2

template <class IteratorT, class StrFn>
int GetMaxWidth(wxWindow *win, IteratorT begin, IteratorT end, StrFn sfn)
{
	int max_width = 0;
	for( ; begin != end(); ++begin ) {
		int this_width = 0;
		int this_height = 0;
		win->GetTextExtent(wxString(sfn(*begin).c_str(), wxConvUTF8),
			&this_width, &this_height);

		max_width = std::max(max_width, this_width);
	}

	return max_width;
}

std::string GetBaseFilename(const std::string &filename);
wxString GetImageFilename(const wxString &filename);
wxString GetButtonFilename(int id, int state);
bool IsButtonEnabled(int id);
class wxDatePickerCtrl;
void MakeDateRecent(bool checked, wxDatePickerCtrl *picker);

bool IsParsable(const std::string &dbname);
bool IsBuildable(const std::string &dbname);

// Determine parsable classes via template specialization
template <class RecordT>
inline bool IsParsable()
{
	return false;
}
#undef HANDLE_PARSER
#define HANDLE_PARSER(dbname) \
	template <> \
	inline bool IsParsable<Barry::dbname>() \
	{ \
		return true; \
	}
ALL_KNOWN_PARSER_TYPES

// Determine buildable classes via template specialization
template <class RecordT>
inline bool IsBuildable()
{
	return false;
}
#undef HANDLE_BUILDER
#define HANDLE_BUILDER(dbname) \
	template <> \
	inline bool IsBuildable<Barry::dbname>() \
	{ \
		return true; \
	}
ALL_KNOWN_BUILDER_TYPES

#endif

