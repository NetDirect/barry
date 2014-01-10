///
/// \file	ipc.h
///		Common things needed for both client and server
///

/*
    Copyright (C) 2010-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_IPC_H__
#define __BARRYDESKTOP_IPC_H__

#include <wx/ipc.h>

#define SERVER_SERVICE_NAME _T("/tmp/bsyncjail.socket")

// Connection Topics
#define TOPIC_STATUS	_T("Status")
#define TOPIC_CONFLICT	_T("Conflict")

// Status items
#define STATUS_ITEM_ERROR	_T("Error")
#define STATUS_ITEM_ENGINE	_T("Engine")
#define STATUS_ITEM_MAPPING	_T("Mapping")
#define STATUS_ITEM_ENTRY	_T("Entry")
#define STATUS_ITEM_MEMBER	_T("Member")

// Special engine messages
#define ENGINE_STATUS_SLOW_SYNC	_T("SLOW_SYNC: on")

// Conflict items
#define CONFLICT_ITEM_START	_T("Start")
#define CONFLICT_ITEM_CHANGE	_T("Change")
#define CONFLICT_ITEM_ANSWER	_T("Answer")

// Used to convert const char* strings into temporary writable
// buffers, since wxConnection is not very const correct.
class SillyBuffer
{
	wxChar *m_buf;
	int m_size;
	int m_data_size;

private:
	// not meant to be copied
	SillyBuffer(const SillyBuffer &);
	SillyBuffer& operator=(const SillyBuffer &);

public:
	SillyBuffer()
		: m_buf( new wxChar[100] )
		, m_size(100)
		, m_data_size(0)
	{
	}

	~SillyBuffer()
	{
		delete [] m_buf;
	}

	int size() const { return m_data_size; }
	wxChar* buf() { return m_buf; }

	void resize(int len)
	{
		if( len+1 > m_size ) {
			delete [] m_buf;
			m_buf = 0;
			m_buf = new wxChar[len+1];
			m_size = len+1;
			m_data_size = 0;
		}
	}

	SillyBuffer& sbuf(const wxString &str) { buf(str); return *this; }
	SillyBuffer& sbuf(const char *str) { buf(str); return *this; }
	SillyBuffer& sbuf(const std::string &str) { buf(str); return *this; }

	wxChar* buf(const wxString &str)
	{
		resize(str.Len());
		memcpy(m_buf, str.GetData(), str.Len() * sizeof(wxChar));
		m_buf[str.Len()] = 0;
		m_data_size = str.Len();
		return m_buf;
	}

	wxChar* buf(const char *str)
	{
		return buf(wxString(str, wxConvUTF8));
	}

	wxChar* buf(const std::string &str)
	{
		return buf(wxString(str.c_str(), wxConvUTF8, str.size()));
	}
};

extern SillyBuffer sb;

#endif

