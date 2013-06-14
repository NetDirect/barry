///
/// \file	ModemDlg.h
///		Dialog class to handle modem functionality
///

/*
    Copyright (C) 2012-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_MODEM_DLG_H__
#define __BARRYDESKTOP_MODEM_DLG_H__

#include <wx/wx.h>
#include <wx/image.h>
#include <vector>
#include <string>
// begin wxGlade: ::dependencies
// end wxGlade


// begin wxGlade: ::extracode
// end wxGlade

namespace Barry {
	class Pin;
}

class ModemDlg : public wxDialog
{
public:
	// begin wxGlade: ModemDlg::ids
	// end wxGlade

private:
	// begin wxGlade: ModemDlg::methods
	void set_properties();
	void do_layout();
	// end wxGlade

protected:
	// begin wxGlade: ModemDlg::attributes
	wxStaticBox* sizer_5_staticbox;
	wxStaticBox* sizer_1_staticbox;
	wxStaticText* label_2;
	wxStaticText* device_label;
	wxListBox* list_box_1;
	wxStaticText* label_1;
	wxTextCtrl* text_ctrl_1;
	// end wxGlade

	wxSizer *bottom_buttons;
	wxSizer *m_top_sizer;

public:
	ModemDlg(wxWindow* parent, const std::vector<std::string> &peers,
		const std::string &default_peer,
		const Barry::Pin &pin);

	std::string GetPeerName() const;
	std::string GetPassword() const;

	static void DoModem(wxWindow *parent, const Barry::Pin &pin);
}; // wxGlade: end class


#endif // MODEMDLG_H
