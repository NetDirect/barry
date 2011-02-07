///
/// \file	ContactEditDlg.h
///		Dialog class to handle the editing of all Barry record classes
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_RECORD_EDIT_DLG_H__
#define __BARRYDESKTOP_RECORD_EDIT_DLG_H__

#include "StringSync.h"
#include <wx/wx.h>
#include <barry/barry.h>
#include <tr1/memory>
#include <list>
// begin wxGlade: ::dependencies
#include <wx/statline.h>
// end wxGlade

// begin wxGlade: ::extracode
// end wxGlade

class ContactEditDlg : public wxDialog
{
public:
	// begin wxGlade: ContactEditDlg::ids
	// end wxGlade

private:
	Barry::Contact &m_rec;
	StringSync m_strings;

	// begin wxGlade: ContactEditDlg::methods
	void set_properties();
	void do_layout();
	// end wxGlade

protected:
	// begin wxGlade: ContactEditDlg::attributes
	wxStaticBox* sizer_8_staticbox;
	wxStaticBox* sizer_7_staticbox;
	wxStaticBox* sizer_6_staticbox;
	wxStaticBox* sizer_5_staticbox;
	wxStaticBox* sizer_9_staticbox;
	wxStaticText* label_13;
	wxTextCtrl* Prefix;
	wxStaticText* FirstNameStatic;
	wxTextCtrl* FirstName;
	wxStaticText* LastNameStatic;
	wxTextCtrl* LastName;
	wxStaticText* label_14;
	wxTextCtrl* Company;
	wxStaticText* label_15;
	wxTextCtrl* JobTitle;
	wxStaticText* label_9;
	wxTextCtrl* Nickname;
	wxStaticText* label_1;
	wxTextCtrl* HomeAddress1;
	wxTextCtrl* HomeAddress2;
	wxTextCtrl* HomeAddress3;
	wxStaticText* label_2;
	wxTextCtrl* HomeCity;
	wxStaticText* label_3;
	wxTextCtrl* HomeProvince;
	wxStaticText* label_4;
	wxTextCtrl* HomePostalCode;
	wxStaticText* label_5;
	wxTextCtrl* HomeCountry;
	wxStaticLine* static_line_1;
	wxStaticLine* static_line_2;
	wxStaticText* label_6;
	wxTextCtrl* HomePhone;
	wxStaticText* label_7;
	wxTextCtrl* HomePhone2;
	wxStaticText* label_8;
	wxTextCtrl* HomeFax;
	wxStaticText* label_1_copy;
	wxTextCtrl* WorkAddress1;
	wxTextCtrl* WorkAddress2;
	wxTextCtrl* WorkAddress3;
	wxStaticText* label_2_copy;
	wxTextCtrl* WorkCity;
	wxStaticText* label_3_copy;
	wxTextCtrl* WorkProvince;
	wxStaticText* label_4_copy;
	wxTextCtrl* WorkPostalCode;
	wxStaticText* label_5_copy;
	wxTextCtrl* WorkCountry;
	wxStaticLine* static_line_1_copy;
	wxStaticLine* static_line_2_copy;
	wxStaticText* label_6_copy;
	wxTextCtrl* WorkPhone;
	wxStaticText* label_7_copy;
	wxTextCtrl* WorkPhone2;
	wxStaticText* label_8_copy;
	wxTextCtrl* WorkFax;
	wxStaticText* label_10;
	wxTextCtrl* MobilePhone;
	wxStaticText* label_11;
	wxTextCtrl* MobilePhone2;
	wxStaticText* label_12;
	wxTextCtrl* Pager;
	wxTextCtrl* Notes;
	wxStaticText* label_16;
	wxTextCtrl* Url;
	// end wxGlade

	wxSizer *bottom_buttons;

public:
	ContactEditDlg(wxWindow *parent, Barry::Contact &rec, bool editable);
	int ShowModal();
};

#endif

