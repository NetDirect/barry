///
/// \file	StringSync.h
///		Class to easily manage the wxString / std::string boundary
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_STRING_SYNC_H__
#define __BARRYDESKTOP_STRING_SYNC_H__

#include <wx/wx.h>
#include <list>

//
// StringSync
//
/// Provides a wxString or std::string while maintaining a link to
/// a corresponding external string of the opposite kind.  When
/// Sync() is called, the external linked string is updated with the
/// current contents of the returned string.
///
class StringSync
{
public:
	typedef std::pair<wxString, std::string*>	WxIsCopyType;
	typedef std::pair<std::string, wxString*>	StdIsCopyType;
	typedef std::list<WxIsCopyType>			WxIsCopyList;
	typedef std::list<StdIsCopyType>		StdIsCopyList;

private:
	WxIsCopyList m_wx;
	StdIsCopyList m_std;

public:
	/// On destruction, calls Sync()
	~StringSync();

	/// Creates an internal wxString, copies the contents of source
	/// into it, and returns its reference.
	wxString* Add(std::string &source);

	/// Does the opposite
	std::string* Add(wxString &source);

	/// Copies all internal wxString contents into the corresponding
	/// external std::strings.
	void SyncToStd();

	/// Copies all internal std::string contents into the corresponding
	/// external wxStrings.
	void SyncToWx();

	/// Calls SyncToStd() and then SyncToWx()
	void Sync();
};

#endif

