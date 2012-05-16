///
/// \file	MimeExportDlg.cc
///		Dialog class to handle viewing/exporting of MIME card data
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

#include "MimeExportDlg.h"
#include "windowids.h"
#include <iostream>
#include <fstream>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
// MimeExportDlg class

BEGIN_EVENT_TABLE(MimeExportDlg, wxDialog)
	EVT_BUTTON	(Dialog_MimeExport_SaveButton,
				MimeExportDlg::OnSaveButton)
END_EVENT_TABLE()

MimeExportDlg::MimeExportDlg(wxWindow* parent,
				const std::string &vdata,
				const std::string &file_types)
	: wxDialog(parent, Dialog_MimeExport, _T("MIME Card Data"))
	, m_vdata(vdata)
	, m_file_types(file_types)
{
	bottom_buttons = CreateButtonSizer(wxOK);

	text_ctrl_1 = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	save_button = new wxButton(this, Dialog_MimeExport_SaveButton, _T("Save..."));

	set_properties();
	do_layout();
}


void MimeExportDlg::set_properties()
{
	text_ctrl_1->SetMinSize(wxSize(400, 240));
	text_ctrl_1->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_vdata)));
}


void MimeExportDlg::do_layout()
{
	wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
	sizer_1->Add(text_ctrl_1, 2, wxALL|wxEXPAND, 10);
	bottom_buttons->Insert(0, save_button, 0, wxRIGHT, 5);
	sizer_1->Add(bottom_buttons, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 10);

	SetSizer(sizer_1);
	sizer_1->Fit(this);
	Layout();
}

void MimeExportDlg::OnSaveButton(wxCommandEvent &event)
{
	wxString ftypes(m_file_types.c_str(), wxConvUTF8);

	wxFileDialog dlg(this, _T("Save Card as..."), _T(""), _T(""),
		ftypes,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_PREVIEW);
	if( dlg.ShowModal() == wxID_OK ) {
		ofstream ofs(dlg.GetPath().utf8_str(), ios::binary);
		ofs.write(m_vdata.data(), m_vdata.size());
		ofs.close();
	}
}

