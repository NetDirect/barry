///
/// \file	PromptDlg.h
///		Dialog wrapper class for generic prompt dialog
///

/*
    Copyright (C) 2007-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_PROMPTDLG_H__
#define __BARRYBACKUP_PROMPTDLG_H__

#include <gtkmm.h>
#include <memory>
#include <string>

class PromptDlg
{
	// Widgets
	std::auto_ptr<Gtk::Dialog> m_pDialog;
	Gtk::Label *m_pPromptLabel;
	Gtk::Entry *m_pPromptEntry;

	// data
	std::string m_answer;

public:
	PromptDlg();
	~PromptDlg();

	void SetPrompt(const std::string &question);
	const char *GetAnswer() const { return m_answer.c_str(); }

	int run();
};

#endif

