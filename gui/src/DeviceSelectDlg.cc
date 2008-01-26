///
/// \file	DeviceSelectDlg.cc
///		Dialog wrapper class for user selection of multiple
///		BlackBerry devices on a USB bus.
///

/*
    Copyright (C) 2007-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "DeviceSelectDlg.h"
#include "util.h"
#include <barry/barry.h>
#include <sstream>
#include <iostream>

DeviceSelectDlg::DeviceSelectDlg(const Barry::Probe &probe)
	: m_pTree(0)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = LoadXml("DeviceSelectDlg.glade");

	Gtk::Dialog *pD = 0;
	xml->get_widget("DeviceSelectDlg", pD);
	m_pDialog.reset(pD);

	xml->get_widget("treeview1", m_pTree);

	m_pListStore = Gtk::ListStore::create(m_Columns);
	LoadTree(probe);
}

DeviceSelectDlg::~DeviceSelectDlg()
{
}

void DeviceSelectDlg::LoadTree(const Barry::Probe &probe)
{
	for( int i = 0; i < probe.GetCount(); i++ ) {
		Gtk::TreeModel::iterator row = m_pListStore->append();
		(*row)[m_Columns.m_pin] = probe.Get(i).m_pin;

		std::ostringstream oss;
		oss << std::hex << probe.Get(i).m_pin;
		(*row)[m_Columns.m_pin_text] = oss.str();
	}
	m_pTree->set_model(m_pListStore);
	m_pTree->append_column("Device PIN", m_Columns.m_pin_text);
}

int DeviceSelectDlg::run()
{
	for(;;) {
		int ret = m_pDialog->run();
		if( ret == Gtk::RESPONSE_OK ) {
			if( m_pTree->get_selection()->get_selected() ) {
				m_pin = (*m_pTree->get_selection()->get_selected())[m_Columns.m_pin];
				return ret;
			}

			m_pin = 0;
			Gtk::MessageDialog msg("Please select a device.");
			msg.run();
		}
		else {
			return ret;
		}
	}
}

