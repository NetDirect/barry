///
/// \file	BaseFrame.h
///		Class for the fixed-size frame that holds the main app
///

/*
    Copyright (C) 2009-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_BASEFRAME_H__
#define __BARRYDESKTOP_BASEFRAME_H__

#include <wx/wx.h>
#include <wx/process.h>
#include <memory>
#include <barry/pin.h>
#include "exechelper.h"

#define MAIN_HEADER_OFFSET 40

class MainMenuMode;
class SyncMode;
class ClickableImage;
class Mode;

class BaseFrame : public wxFrame, public TermCatcher
{
private:
	DECLARE_EVENT_TABLE()

private:
	std::auto_ptr<wxBitmap> m_background;
	std::auto_ptr<MainMenuMode> m_main_menu_mode;// only this mode is
							// never reset()
	std::auto_ptr<SyncMode> m_sync_mode;
	std::auto_ptr<ClickableImage> m_barry_logo, m_netdirect_logo;
	std::auto_ptr<wxMenu> m_sysmenu;
	std::auto_ptr<wxComboBox> m_device_combo;
	std::auto_ptr<wxButton> m_back_button;
	int m_width, m_height;

	Mode *m_current_mode;

	ExecHelper m_backup_process;

public:
	BaseFrame(const wxImage &background);

	// utility functions
	void UpdateMenuState();
	void CreateDeviceCombo(Barry::Pin pin = Barry::Pin());
	Barry::Pin GetCurrentComboPin();
	void EnableBackButton(Mode *new_mode);
	void DisableBackButton();	// also returns to the main menu

	// events
	void OnSize(wxSizeEvent &event);
	void OnPaint(wxPaintEvent &event);
	void OnMouseMotion(wxMouseEvent &event);
	void OnLeftDown(wxMouseEvent &event);
	void OnLeftUp(wxMouseEvent &event);
	void OnBackupRestore(wxCommandEvent &event);
	void OnSync(wxCommandEvent &event);
	void OnModem(wxCommandEvent &event);
	void OnBackButton(wxCommandEvent &event);
	void OnTermBackupAndRestore(wxProcessEvent &event);
	void OnBarryLogoClicked(wxCommandEvent &event);
	void OnNetDirectLogoClicked(wxCommandEvent &event);
	void OnDeviceComboChange(wxCommandEvent &event);

	// sys menu (triggered by the Barry logo)
	void OnVerboseLogging(wxCommandEvent &event);
	void OnResetDevice(wxCommandEvent &event);
	void OnRescanUsb(wxCommandEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnExit(wxCommandEvent &event);
};

#endif

