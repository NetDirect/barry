///
/// \file	MemoEditDlg.cc
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

#include "MemoEditDlg.h"
#include "windowids.h"

// begin wxGlade: ::extracode
// end wxGlade


//////////////////////////////////////////////////////////////////////////////
// MemoEditDlg class

MemoEditDlg::MemoEditDlg(wxWindow* parent,
			Barry::Memo &rec,
			bool editable)
	: wxDialog(parent, Dialog_MemoEdit, _T("Memo Record"))
	, m_rec(rec)
{
	m_rec.Categories.CategoryList2Str(m_category_list);

	if( editable ) {
		bottom_buttons = CreateButtonSizer(wxOK | wxCANCEL);
	}
	else {
		bottom_buttons = CreateButtonSizer(wxCANCEL);
	}

	// begin wxGlade: MemoEditDlg::MemoEditDlg
	label_1 = new wxStaticText(this, wxID_ANY, wxT("Title:"));
	text_ctrl_2 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_2 = new wxStaticText(this, wxID_ANY, wxT("Categories:"));
	text_ctrl_3 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	text_ctrl_1 = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

	set_properties();
	do_layout();
	// end wxGlade
}


void MemoEditDlg::set_properties()
{
	// begin wxGlade: MemoEditDlg::set_properties
	SetTitle(wxT("Memo"));
	text_ctrl_2->SetMinSize(wxSize(300, -1));
	text_ctrl_2->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Title)));
	label_2->SetToolTip(wxT("Comma separated"));
	text_ctrl_3->SetMinSize(wxSize(300, -1));
	text_ctrl_3->SetToolTip(wxT("Comma separated list of categories (can be empty)"));
	text_ctrl_3->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_category_list)));
	text_ctrl_1->SetMinSize(wxSize(-1, 240));
	text_ctrl_1->SetToolTip(wxT("Body of memo"));
	text_ctrl_1->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Body)));
	// end wxGlade
}


void MemoEditDlg::do_layout()
{
	// begin wxGlade: MemoEditDlg::do_layout
	wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* grid_sizer_1 = new wxFlexGridSizer(2, 2, 10, 5);
	grid_sizer_1->Add(label_1, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(text_ctrl_2, 1, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(label_2, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(text_ctrl_3, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 0);
	sizer_1->Add(grid_sizer_1, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 10);
	sizer_1->Add(text_ctrl_1, 2, wxALL|wxEXPAND, 10);
	// end wxGlade

	sizer_1->Add(bottom_buttons, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);

	// NOTE: watch that wxGlade doesn't add these again above, if
	// you regenerate the code... we need this here, so that we
	// can add the button spacer.
	SetSizer(sizer_1);
	sizer_1->Fit(this);
	Layout();
}

bool MemoEditDlg::TransferDataFromWindow()
{
	if( !wxDialog::TransferDataFromWindow() )
		return false;

	m_strings.Sync();
	m_rec.Categories.CategoryStr2List(m_category_list);

	// make sure that the title contains something
	if( !m_rec.Title.size() ) {
		wxMessageBox(_T("Please enter a title for your memo."),
			_T("Required Fields"),
			wxOK | wxICON_INFORMATION, this);
		return false;
	}

	return true;
}

