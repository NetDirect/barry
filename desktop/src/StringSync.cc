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

#include "StringSync.h"

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// StringSync

StringSync::~StringSync()
{
	Sync();
}

wxString* StringSync::Add(std::string &source)
{
	m_wx.push_front(WxIsCopyType(wxString(source.c_str(), wxConvUTF8), &source));
	return &m_wx.begin()->first;
}

std::string* StringSync::Add(wxString &source)
{
	m_std.push_front(StdIsCopyType(std::string(source.utf8_str()), &source));
	return &m_std.begin()->first;
}

void StringSync::SyncToStd()
{
	WxIsCopyList::iterator b = m_wx.begin();
	for( ; b != m_wx.end(); ++b ) {
		b->second->assign(b->first.utf8_str());
	}
}

void StringSync::SyncToWx()
{
	StdIsCopyList::iterator b = m_std.begin();
	for( ; b != m_std.end(); ++b ) {
		*b->second = wxString(b->first.c_str(), wxConvUTF8);
	}
}

void StringSync::Sync()
{
	SyncToStd();
	SyncToWx();
}

