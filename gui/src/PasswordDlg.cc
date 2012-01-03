///
/// \file	PasswordDlg.cc
///		Dialog wrapper class for password entry
///

/*
    Copyright (C) 2007-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "PasswordDlg.h"
#include "util.h"
#include "i18n.h"
#include <sstream>

PasswordDlg::PasswordDlg(int remaining_tries)
	: m_pPromptLabel(0),
	m_pPasswordEntry(0)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = LoadXml("PasswordDlg.glade");

	Gtk::Dialog *pD = 0;
	xml->get_widget("PasswordDlg", pD);
	m_pDialog.reset(pD);

	xml->get_widget("prompt_label", m_pPromptLabel);
	xml->get_widget("password_entry", m_pPasswordEntry);

	std::ostringstream oss;
	oss << _("Please enter device password: (") << remaining_tries << _(" tries remaining)");
	m_pPromptLabel->set_text(oss.str());
}

PasswordDlg::~PasswordDlg()
{
	// do our part in blanking passwords in memory...
	for( size_t i = 0; i < m_password.size(); i++ ) {
		m_password[i] = 0;
	}
}

int PasswordDlg::run()
{
	int ret = m_pDialog->run();
	if( ret == Gtk::RESPONSE_OK ) {
		m_password = m_pPasswordEntry->get_text();
	}
	return ret;
}

