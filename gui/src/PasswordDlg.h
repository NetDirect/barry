///
/// \file	PasswordDlg.h
///		Dialog wrapper class for password entry
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

#ifndef __BARRYBACKUP_PASSWORDDLG_H__
#define __BARRYBACKUP_PASSWORDDLG_H__

#include <gtkmm.h>
#include <memory>
#include <string>

class PasswordDlg
{
	// Widgets
	std::auto_ptr<Gtk::Dialog> m_pDialog;
	Gtk::Label *m_pPromptLabel;
	Gtk::Entry *m_pPasswordEntry;

	// data
	std::string m_password;

public:
	explicit PasswordDlg(int remaining_tries);
	~PasswordDlg();

	const char *GetPassword() const { return m_password.c_str(); }

	int run();
};

#endif

