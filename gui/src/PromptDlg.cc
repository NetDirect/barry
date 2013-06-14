///
/// \file	PromptDlg.cc
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

#include "PromptDlg.h"
#include "util.h"

PromptDlg::PromptDlg()
	: m_pPromptLabel(0),
	m_pPromptEntry(0)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = LoadXml("PromptDlg.glade");

	Gtk::Dialog *pD = 0;
	xml->get_widget("PromptDlg", pD);
	m_pDialog.reset(pD);

	xml->get_widget("prompt_label", m_pPromptLabel);
	xml->get_widget("prompt_entry", m_pPromptEntry);
}

PromptDlg::~PromptDlg()
{
}

void PromptDlg::SetPrompt(const std::string &question)
{
	m_pPromptLabel->set_text(question);
}

int PromptDlg::run()
{
	int ret = m_pDialog->run();
	if( ret == Gtk::RESPONSE_OK ) {
		m_answer = m_pPromptEntry->get_text();
	}
	return ret;
}

