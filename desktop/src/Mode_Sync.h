///
/// \file	Mode_Sync.h
///		Mode derived class for syncing
///

/*
    Copyright (C) 2009-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_MODE_SYNC_H__
#define __BARRYDESKTOP_MODE_SYNC_H__

#include "Mode.h"
#include "deviceset.h"
#include "configui.h"

class SyncMode : public wxEvtHandler, public Mode
{
private:
	DECLARE_EVENT_TABLE()

private:
	wxWindow *m_parent;

	std::auto_ptr<DeviceSet> m_device_set;
	ConfigUI::ptr m_cui;

	// window controls
	std::auto_ptr<wxBoxSizer> m_topsizer;
	std::auto_ptr<wxButton> m_sync_now_button;
	std::auto_ptr<wxButton> m_configure_button;
	std::auto_ptr<wxButton> m_run_app_button;
	std::auto_ptr<wxButton> m_1way_reset_button;
	std::auto_ptr<wxListCtrl> m_device_list;
	std::auto_ptr<wxStaticText> m_label[4];

protected:
	static std::string Timestamp(time_t last_sync);

	int GetMaxTimestampWidth(wxWindow *win);
	void FillDeviceList();
	void UpdateButtons();
	DeviceSet::subset_type GetSelectedDevices();
	void ReselectDevices(const DeviceSet::subset_type &set);
	void ConfigureDevice(int device_index);
	void ConfigureDevice(DeviceEntry &entry);
	void CheckConfigured(DeviceSet::subset_type &subset);
	void RefillList();
	int GetSelectedDevice();	// returns index, or -1 if none or
					// more than one selected... also
					// handles the message box
	int GetAuthoritativeSide(int device_index);
	bool ZapConflicts(int device_index, int authoritative_side);
	void RewriteConfig(int device_index);
	bool WarnAbout1WayReset();

public:
	SyncMode(wxWindow *parent);
	~SyncMode();

	// virtual override events (derived from Mode)
	wxString GetTitleText() const { return _T("Barry Sync"); }

	// window events
	void OnSyncNow(wxCommandEvent &event);
	void OnConfigure(wxCommandEvent &event);
	void OnRunApp(wxCommandEvent &event);
	void On1WayReset(wxCommandEvent &event);
	void OnListSelChange(wxListEvent &event);//to keep track of button state
	void OnConfigureDevice(wxListEvent &event);
};

#endif

