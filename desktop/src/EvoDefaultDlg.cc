///
/// \file	EvoDefaultDlg.cc
///		Successful defaults detected dialog
///		The configuration dialog used to configure Evolution sources
///

/*
    Copyright (C) 2011-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "EvoDefaultDlg.h"
#include "windowids.h"
#include <wx/statline.h>
#include "wxi18n.h"

using namespace std;

BEGIN_EVENT_TABLE(EvoDefaultDlg, wxDialog)
	EVT_BUTTON	(Dialog_EvoDefault_ManualConfigButton,
				EvoDefaultDlg::OnManualButton)
END_EVENT_TABLE()

EvoDefaultDlg::EvoDefaultDlg(wxWindow *parent)
	: wxDialog(parent, Dialog_EvoDefault, _W("Evolution Success"))
{
	CreateLayout();
}

void EvoDefaultDlg::CreateLayout()
{
	wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer *msgsizer = new wxBoxSizer(wxHORIZONTAL);

	// I'd love to add a portable, system ICON_INFORMATION here,
	// but I don't know how in wxWidgets :-(   Please send me a patch.
	//msgsizer->Add( someicon );

	msgsizer->Add(
		new wxStaticText(this, wxID_ANY, _W("Successfully auto-detected Evolution configuration!")),
		0, wxALIGN_LEFT | wxTOP | wxLEFT | wxRIGHT, 10);

	topsizer->Add(msgsizer, 0, 0, 0);
	topsizer->Add( new wxStaticLine(this),
		0, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 10);

	wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
	buttons->Add( new wxButton(this, Dialog_EvoDefault_ManualConfigButton,
			_W("Manual Cfg...")),
		0, wxALIGN_LEFT | wxEXPAND, 0);
	buttons->AddStretchSpacer(1);

	wxButton *ok = new wxButton(this, wxID_OK, _T("Ok"));
	ok->SetDefault();
	buttons->Add( ok, 0, wxALIGN_RIGHT | wxEXPAND, 0);

	topsizer->Add(buttons, 0, wxALL | wxEXPAND | wxALIGN_RIGHT, 10);

	SetSizer(topsizer);
	topsizer->SetSizeHints(this);
	topsizer->Layout();
}

void EvoDefaultDlg::OnManualButton(wxCommandEvent &event)
{
	EndModal(Dialog_EvoDefault_ManualConfigButton);
}

