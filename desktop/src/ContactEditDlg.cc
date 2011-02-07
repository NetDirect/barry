///
/// \file	ContactEditDlg.cc
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

#include "ContactEditDlg.h"
#include "windowids.h"

// begin wxGlade: ::extracode
// end wxGlade

ContactEditDlg::ContactEditDlg(wxWindow *parent,
				Barry::Contact &rec,
				bool editable)
	: wxDialog(parent, Dialog_ContactEdit, _T("Contact Record"))
	, m_rec(rec)
{
	if( editable ) {
		bottom_buttons = CreateButtonSizer(wxOK | wxCANCEL);
	}
	else {
		bottom_buttons = CreateButtonSizer(wxCANCEL);
	}

	// begin wxGlade: ContactEditDlg::ContactEditDlg
	sizer_5_staticbox = new wxStaticBox(this, -1, wxT("Home"));
	sizer_6_staticbox = new wxStaticBox(this, -1, wxT("Work"));
	sizer_7_staticbox = new wxStaticBox(this, -1, wxT("Mobile"));
	sizer_8_staticbox = new wxStaticBox(this, -1, wxT("Notes"));
	sizer_9_staticbox = new wxStaticBox(this, -1, wxT("Name"));
	label_13 = new wxStaticText(this, wxID_ANY, wxT("Title"));
	Prefix = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	FirstNameStatic = new wxStaticText(this, wxID_ANY, wxT("First"));
	FirstName = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	LastNameStatic = new wxStaticText(this, wxID_ANY, wxT("Last"));
	LastName = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_14 = new wxStaticText(this, wxID_ANY, wxT("Company"));
	Company = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_15 = new wxStaticText(this, wxID_ANY, wxT("Job Title"));
	JobTitle = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_9 = new wxStaticText(this, wxID_ANY, wxT("Nickname"));
	Nickname = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_1 = new wxStaticText(this, wxID_ANY, wxT("Address"));
	HomeAddress1 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	HomeAddress2 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	HomeAddress3 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_2 = new wxStaticText(this, wxID_ANY, wxT("City"));
	HomeCity = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_3 = new wxStaticText(this, wxID_ANY, wxT("Province"));
	HomeProvince = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_4 = new wxStaticText(this, wxID_ANY, wxT("Postal Code"));
	HomePostalCode = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_5 = new wxStaticText(this, wxID_ANY, wxT("Country"));
	HomeCountry = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	static_line_1 = new wxStaticLine(this, wxID_ANY);
	static_line_2 = new wxStaticLine(this, wxID_ANY);
	label_6 = new wxStaticText(this, wxID_ANY, wxT("Phone"));
	HomePhone = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_7 = new wxStaticText(this, wxID_ANY, wxT("Phone 2"));
	HomePhone2 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_8 = new wxStaticText(this, wxID_ANY, wxT("Fax"));
	HomeFax = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_1_copy = new wxStaticText(this, wxID_ANY, wxT("Address"));
	WorkAddress1 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	WorkAddress2 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	WorkAddress3 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_2_copy = new wxStaticText(this, wxID_ANY, wxT("City"));
	WorkCity = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_3_copy = new wxStaticText(this, wxID_ANY, wxT("Province"));
	WorkProvince = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_4_copy = new wxStaticText(this, wxID_ANY, wxT("Postal Code"));
	WorkPostalCode = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_5_copy = new wxStaticText(this, wxID_ANY, wxT("Country"));
	WorkCountry = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	static_line_1_copy = new wxStaticLine(this, wxID_ANY);
	static_line_2_copy = new wxStaticLine(this, wxID_ANY);
	label_6_copy = new wxStaticText(this, wxID_ANY, wxT("Phone"));
	WorkPhone = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_7_copy = new wxStaticText(this, wxID_ANY, wxT("Phone 2"));
	WorkPhone2 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_8_copy = new wxStaticText(this, wxID_ANY, wxT("Fax"));
	WorkFax = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_10 = new wxStaticText(this, wxID_ANY, wxT("Cell"));
	MobilePhone = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_11 = new wxStaticText(this, wxID_ANY, wxT("Cell 2"));
	MobilePhone2 = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	label_12 = new wxStaticText(this, wxID_ANY, wxT("Pager"));
	Pager = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	Notes = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_MULTILINE);
	label_16 = new wxStaticText(this, wxID_ANY, wxT("URL"));
	Url = new wxTextCtrl(this, wxID_ANY, wxEmptyString);

	set_properties();
	do_layout();
	// end wxGlade
}

void ContactEditDlg::set_properties()
{
	// begin wxGlade: ContactEditDlg::set_properties
	SetTitle(wxT("Contact"));
	Prefix->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Prefix)));
	FirstName->SetMinSize(wxSize(100, -1));
	FirstName->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.FirstName)));
	LastName->SetMinSize(wxSize(100, -1));
	LastName->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.LastName)));
	Company->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Company)));
	JobTitle->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.JobTitle)));
	Nickname->SetMinSize(wxSize(100, -1));
	Nickname->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Nickname)));
	HomeAddress1->SetMinSize(wxSize(170, -1));
	HomeAddress1->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomeAddress.Address1)));
	HomeAddress2->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomeAddress.Address2)));
	HomeAddress3->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomeAddress.Address3)));
	HomeCity->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomeAddress.City)));
	HomeProvince->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomeAddress.Province)));
	HomePostalCode->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomeAddress.PostalCode)));
	HomeCountry->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomeAddress.Country)));
	HomePhone->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomePhone)));
	HomePhone2->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomePhone2)));
	HomeFax->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.HomeFax)));
	WorkAddress1->SetMinSize(wxSize(170, -1));
	WorkAddress1->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkAddress.Address1)));
	WorkAddress2->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkAddress.Address2)));
	WorkAddress3->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkAddress.Address3)));
	WorkCity->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkAddress.City)));
	WorkProvince->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkAddress.Province)));
	WorkPostalCode->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkAddress.PostalCode)));
	WorkCountry->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkAddress.Country)));
	WorkPhone->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkPhone)));
	WorkPhone2->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.WorkPhone2)));
	WorkFax->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Fax)));
	MobilePhone->SetMinSize(wxSize(100, -1));
	MobilePhone->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.MobilePhone)));
	MobilePhone2->SetMinSize(wxSize(100, -1));
	MobilePhone2->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.MobilePhone2)));
	Pager->SetMinSize(wxSize(100, -1));
	Pager->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Pager)));
	Notes->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.Notes)));
	Url->SetValidator(wxTextValidator(wxFILTER_NONE, m_strings.Add(m_rec.URL)));
	// end wxGlade
}


void ContactEditDlg::do_layout()
{
	// begin wxGlade: ContactEditDlg::do_layout
	wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* sizer_8 = new wxStaticBoxSizer(sizer_8_staticbox, wxVERTICAL);
	wxBoxSizer* sizer_10 = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* sizer_7 = new wxStaticBoxSizer(sizer_7_staticbox, wxHORIZONTAL);
	wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* sizer_6 = new wxStaticBoxSizer(sizer_6_staticbox, wxHORIZONTAL);
	wxFlexGridSizer* grid_sizer_1_copy = new wxFlexGridSizer(11, 2, 1, 3);
	wxStaticBoxSizer* sizer_5 = new wxStaticBoxSizer(sizer_5_staticbox, wxHORIZONTAL);
	wxFlexGridSizer* grid_sizer_1 = new wxFlexGridSizer(11, 2, 1, 3);
	wxStaticBoxSizer* sizer_9 = new wxStaticBoxSizer(sizer_9_staticbox, wxVERTICAL);
	wxFlexGridSizer* grid_sizer_2 = new wxFlexGridSizer(2, 6, 2, 3);
	grid_sizer_2->Add(label_13, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	grid_sizer_2->Add(Prefix, 0, wxEXPAND, 0);
	grid_sizer_2->Add(FirstNameStatic, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 10);
	grid_sizer_2->Add(FirstName, 1, wxEXPAND, 0);
	grid_sizer_2->Add(LastNameStatic, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 10);
	grid_sizer_2->Add(LastName, 1, wxEXPAND, 0);
	grid_sizer_2->Add(label_14, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	grid_sizer_2->Add(Company, 0, wxEXPAND, 0);
	grid_sizer_2->Add(label_15, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 10);
	grid_sizer_2->Add(JobTitle, 0, wxEXPAND, 0);
	grid_sizer_2->Add(label_9, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 10);
	grid_sizer_2->Add(Nickname, 1, wxEXPAND, 0);
	grid_sizer_2->AddGrowableCol(1);
	grid_sizer_2->AddGrowableCol(3);
	grid_sizer_2->AddGrowableCol(5);
	sizer_9->Add(grid_sizer_2, 1, wxBOTTOM|wxEXPAND, 3);
	sizer_1->Add(sizer_9, 0, wxALL|wxEXPAND, 5);
	grid_sizer_1->Add(label_1, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(HomeAddress1, 0, 0, 0);
	grid_sizer_1->Add(20, 20, 0, 0, 0);
	grid_sizer_1->Add(HomeAddress2, 0, wxEXPAND, 0);
	grid_sizer_1->Add(20, 20, 0, 0, 0);
	grid_sizer_1->Add(HomeAddress3, 0, wxEXPAND, 0);
	grid_sizer_1->Add(label_2, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(HomeCity, 0, wxEXPAND, 0);
	grid_sizer_1->Add(label_3, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(HomeProvince, 0, wxEXPAND, 0);
	grid_sizer_1->Add(label_4, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(HomePostalCode, 0, wxEXPAND, 0);
	grid_sizer_1->Add(label_5, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(HomeCountry, 0, wxEXPAND, 0);
	grid_sizer_1->Add(static_line_1, 0, wxEXPAND, 0);
	grid_sizer_1->Add(static_line_2, 0, wxEXPAND, 0);
	grid_sizer_1->Add(label_6, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(HomePhone, 0, wxEXPAND, 0);
	grid_sizer_1->Add(label_7, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(HomePhone2, 0, wxEXPAND, 0);
	grid_sizer_1->Add(label_8, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1->Add(HomeFax, 0, wxEXPAND, 0);
	sizer_5->Add(grid_sizer_1, 1, wxEXPAND, 0);
	sizer_4->Add(sizer_5, 1, wxLEFT|wxRIGHT|wxEXPAND, 2);
	grid_sizer_1_copy->Add(label_1_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1_copy->Add(WorkAddress1, 0, 0, 0);
	grid_sizer_1_copy->Add(20, 20, 0, 0, 0);
	grid_sizer_1_copy->Add(WorkAddress2, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(20, 20, 0, 0, 0);
	grid_sizer_1_copy->Add(WorkAddress3, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(label_2_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1_copy->Add(WorkCity, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(label_3_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1_copy->Add(WorkProvince, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(label_4_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1_copy->Add(WorkPostalCode, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(label_5_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1_copy->Add(WorkCountry, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(static_line_1_copy, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(static_line_2_copy, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(label_6_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1_copy->Add(WorkPhone, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(label_7_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1_copy->Add(WorkPhone2, 0, wxEXPAND, 0);
	grid_sizer_1_copy->Add(label_8_copy, 0, wxALIGN_CENTER_VERTICAL, 0);
	grid_sizer_1_copy->Add(WorkFax, 0, wxEXPAND, 0);
	sizer_6->Add(grid_sizer_1_copy, 1, wxEXPAND, 0);
	sizer_4->Add(sizer_6, 1, wxLEFT|wxRIGHT|wxEXPAND, 2);
	sizer_1->Add(sizer_4, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	sizer_7->Add(label_10, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	sizer_7->Add(MobilePhone, 1, 0, 0);
	sizer_7->Add(20, 20, 0, 0, 0);
	sizer_7->Add(label_11, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	sizer_7->Add(MobilePhone2, 1, 0, 0);
	sizer_7->Add(20, 20, 0, 0, 0);
	sizer_7->Add(label_12, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	sizer_7->Add(Pager, 1, 0, 0);
	sizer_1->Add(sizer_7, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	sizer_8->Add(Notes, 0, wxEXPAND, 0);
	sizer_10->Add(label_16, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 1);
	sizer_10->Add(Url, 1, wxALL, 2);
	sizer_8->Add(sizer_10, 0, wxEXPAND, 0);
	sizer_1->Add(sizer_8, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	// end wxGlade

	sizer_1->Add(bottom_buttons, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);

	// NOTE: the code generator will generate these 3 calls above.
	// They may be removed above, since it will probably slow
	// down the GUI display.
	SetSizer(sizer_1);
	sizer_1->Fit(this);
	Layout();

}

int ContactEditDlg::ShowModal()
{
	int ret = wxDialog::ShowModal();
	m_strings.Sync();
	return ret;
}

