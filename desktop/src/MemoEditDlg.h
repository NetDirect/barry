///
/// \file	MemoEditDlg.h
///		Dialog class to handle the editing of the Memo record
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_MEMO_EDIT_DLG_H__
#define __BARRYDESKTOP_MEMO_EDIT_DLG_H__


#include "StringSync.h"
#include <wx/wx.h>
#include <barry/barry.h>
// begin wxGlade: ::dependencies
// end wxGlade


// begin wxGlade: ::extracode
// end wxGlade


class MemoEditDlg : public wxDialog
{
private:
	Barry::Memo &m_rec;
	std::string m_category_list;
	StringSync m_strings;

	// begin wxGlade: MemoEditDlg::methods
	void set_properties();
	void do_layout();
	// end wxGlade

protected:
	// begin wxGlade: MemoEditDlg::attributes
	wxStaticText* label_1;
	wxTextCtrl* text_ctrl_2;
	wxStaticText* label_2;
	wxTextCtrl* text_ctrl_3;
	wxTextCtrl* text_ctrl_1;
	// end wxGlade

	wxSizer *bottom_buttons;

public:
	// begin wxGlade: MemoEditDlg::ids
	// end wxGlade

	MemoEditDlg(wxWindow* parent, Barry::Memo &rec, bool editable);

	// in case any validation is required:
	virtual bool TransferDataFromWindow();
}; // wxGlade: end class


#endif // MEMOEDITDLG_H
