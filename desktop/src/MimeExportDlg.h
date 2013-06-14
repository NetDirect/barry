///
/// \file	MimeExportDlg.h
///		Dialog class to handle viewing/exporting of MIME card data
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

#ifndef __BARRYDESKTOP_MIME_EXPORT_DLG_H__
#define __BARRYDESKTOP_MIME_EXPORT_DLG_H__

#include "StringSync.h"
#include <wx/wx.h>
#include <barry/barry.h>


class MimeExportDlg : public wxDialog
{
private:
	std::string m_vdata;
	std::string m_file_types;

	StringSync m_strings;

	void set_properties();
	void do_layout();

protected:
	wxTextCtrl *text_ctrl_1;
	wxButton *save_button;
	wxSizer *bottom_buttons;

protected:
	DECLARE_EVENT_TABLE()	// sets to protected

public:
	MimeExportDlg(wxWindow* parent, const std::string &vdata,
		const std::string &file_types);

	void OnSaveButton(wxCommandEvent &event);
};


#endif

