///
/// \file	DeviceSelectDlg.h
///		Dialog wrapper class for user selection of multiple
///		BlackBerry devices on a USB bus.
///

/*
    Copyright (C) 2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_DEVICESELECTDLG_H__
#define __BARRYBACKUP_DEVICESELECTDLG_H__

#include <gtkmm.h>
#include <memory>
#include <stdint.h>

namespace Barry {
	class Probe;
}

class DeviceSelectDlg
{
	class Columns : public Gtk::TreeModelColumnRecord
	{
	public:
		Gtk::TreeModelColumn<Glib::ustring> m_pin_text;
		Gtk::TreeModelColumn<uint32_t> m_pin;

		Columns()
		{
			add(m_pin_text);
			add(m_pin);
		}
	};

	// Widgets
	std::auto_ptr<Gtk::Dialog> m_pDialog;
	Gtk::TreeView *m_pTree;
	Columns m_Columns;
	Glib::RefPtr<Gtk::ListStore> m_pListStore;

	// data
	uint32_t m_pin;

protected:
	void LoadTree(const Barry::Probe &probe);

public:
	DeviceSelectDlg(const Barry::Probe &probe);
	~DeviceSelectDlg();

	uint32_t GetPIN() const { return m_pin; }

	int run();
};

#endif

