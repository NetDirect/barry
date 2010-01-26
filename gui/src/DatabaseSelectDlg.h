///
/// \file	DatabaseSelectDlg.h
///		Dialog wrapper class for user selection of device databases
///

/*
    Copyright (C) 2007-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_DATABASESELECTDLG_H__
#define __BARRYBACKUP_DATABASESELECTDLG_H__

#include <gtkmm.h>
#include <memory>
#include <barry/barry.h>

namespace Barry {
	class DatabaseDatabase;
}

class DatabaseSelectDlg
{
	class Columns : public Gtk::TreeModelColumnRecord
	{
	public:
		Gtk::TreeModelColumn<bool> m_selected;
		Gtk::TreeModelColumn<Glib::ustring> m_name;

		Columns()
		{
			add(m_selected);
			add(m_name);
		}
	};

	// Widgets
	std::auto_ptr<Gtk::Dialog> m_pDialog;
	Gtk::Label *m_pTopLabel;
	Gtk::TreeView *m_pTree;
	Columns m_Columns;
	Glib::RefPtr<Gtk::ListStore> m_pListStore;

	// data
	Barry::ConfigFile::DBListType m_selections;

protected:
	void LoadTree(const Barry::DatabaseDatabase &dbdb);
	bool IsSelected(const std::string &dbname);
	void SaveSelections();

public:
	DatabaseSelectDlg(const Barry::DatabaseDatabase &dbdb,
		const Barry::ConfigFile::DBListType &selections,
		const Glib::ustring &label);
	~DatabaseSelectDlg();

	const Barry::ConfigFile::DBListType& GetSelections() const { return m_selections; }

	int run();

	// signals
	void on_select_all();
	void on_deselect_all();
};

#endif

