///
/// \file	DatabaseSelectDlg.cc
///		Dialog wrapper class for user selection of device databases
///

/*
    Copyright (C) 2007-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "DatabaseSelectDlg.h"
#include "util.h"
#include "i18n.h"
#include <barry/barry.h>
#include <sstream>

DatabaseSelectDlg::DatabaseSelectDlg(const Barry::DatabaseDatabase &dbdb,
				     const Barry::ConfigFile::DBListType &selections,
				     const Glib::ustring &label)
	: m_pTopLabel(0),
	m_pTree(0),
	m_selections(selections)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = LoadXml("DatabaseSelectDlg.glade");

	Gtk::Dialog *pD = 0;
	xml->get_widget("DatabaseSelectDlg", pD);
	m_pDialog.reset(pD);

	xml->get_widget("toplabel", m_pTopLabel);
	xml->get_widget("treeview1", m_pTree);

	Gtk::Button *pButton = 0;
	xml->get_widget("select_all", pButton);
	pButton->signal_clicked().connect(
		sigc::mem_fun(*this, &DatabaseSelectDlg::on_select_all));

	pButton = 0;
	xml->get_widget("deselect_all", pButton);
	pButton->signal_clicked().connect(
		sigc::mem_fun(*this, &DatabaseSelectDlg::on_deselect_all));

	m_pTopLabel->set_text(label);

	m_pListStore = Gtk::ListStore::create(m_Columns);
	m_pListStore->set_sort_column(m_Columns.m_name, Gtk::SORT_ASCENDING);
	LoadTree(dbdb);
}

DatabaseSelectDlg::~DatabaseSelectDlg()
{
}

void DatabaseSelectDlg::LoadTree(const Barry::DatabaseDatabase &dbdb)
{
	Barry::DatabaseDatabase::DatabaseArrayType::const_iterator i =
		dbdb.Databases.begin();

	for( ; i != dbdb.Databases.end(); ++i ) {
		Gtk::TreeModel::iterator row = m_pListStore->append();
		(*row)[m_Columns.m_selected] = IsSelected(i->Name);
		(*row)[m_Columns.m_name] = i->Name;
	}
	m_pTree->set_model(m_pListStore);
	m_pTree->append_column_editable(_("Active"), m_Columns.m_selected);
	m_pTree->append_column(_("Name"), m_Columns.m_name);
}

bool DatabaseSelectDlg::IsSelected(const std::string &dbname)
{
	return m_selections.IsSelected(dbname);
}

void DatabaseSelectDlg::SaveSelections()
{
	// start fresh
	m_selections.clear();

	// cycle through tree control and add all selected names
	Gtk::TreeModel::Children children = m_pListStore->children();
	Gtk::TreeModel::Children::const_iterator i = children.begin();
	for( ; i != children.end(); ++i ) {
		if( (*i)[m_Columns.m_selected] ) {
			Glib::ustring uname = (*i)[m_Columns.m_name];
			std::string name = uname;
			m_selections.push_back( name );
		}
	}
}

int DatabaseSelectDlg::run()
{
	int ret = m_pDialog->run();
	if( ret == Gtk::RESPONSE_OK ) {
		SaveSelections();
	}
	return ret;
}

void DatabaseSelectDlg::on_select_all()
{
	// cycle through tree control and add all selected names
	Gtk::TreeModel::Children children = m_pListStore->children();
	Gtk::TreeModel::Children::const_iterator i = children.begin();
	for( ; i != children.end(); ++i ) {
		(*i)[m_Columns.m_selected] = true;
	}
}

void DatabaseSelectDlg::on_deselect_all()
{
	// cycle through tree control and add all selected names
	Gtk::TreeModel::Children children = m_pListStore->children();
	Gtk::TreeModel::Children::const_iterator i = children.begin();
	for( ; i != children.end(); ++i ) {
		(*i)[m_Columns.m_selected] = false;
	}
}

