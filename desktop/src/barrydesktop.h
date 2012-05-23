///
/// \file	barrydesktop.h
///		Program entry point for the desktop GUI
///

/*
    Copyright (C) 2009-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_BARRYDESKTOP_H__
#define __BARRYDESKTOP_BARRYDESKTOP_H__

#include <wx/wx.h>
#include <barry/barry.h>
#include <memory>

class wxSplashScreen;
namespace OpenSync {
	class APISet;
}

class UsbScanSplash
{
	std::auto_ptr<wxSplashScreen> m_splash;
public:
	UsbScanSplash();
	~UsbScanSplash();
};

class BarryDesktopApp : public wxApp
{
private:
	Barry::GlobalConfigFile m_global_config;
	Barry::Probe::Results m_results;
	std::auto_ptr<OpenSync::APISet> m_set;

public:
	BarryDesktopApp();

	//
	// data access
	//
	Barry::GlobalConfigFile& GetGlobalConfig() { return m_global_config; }
	const Barry::Probe::Results& GetResults() const { return m_results; }
	OpenSync::APISet& GetOpenSync() { return *m_set; }

	//
	// operations
	//

	void ShowMissingOpenSyncMessage();

	/// Fills m_results with new data after a brand new scan.
	/// Does not catch exceptions.
	void Probe();

	/// Grabs a screenshot of the given device.
	/// Can throw exceptions on error.
	wxBitmap GetScreenshot(const Barry::ProbeResult &device) const;

	/// Sets the device name for the given device PIN.
	/// If the PIN exists in m_results, the associated
	/// m_results entry will be updated with the new device name,
	/// so the next GetResults() will contain the new name.
	void SetDeviceName(Barry::Pin pin, const std::string &name);

	/// Returns device name for the given PIN, but ONLY if the
	/// device is connected.  If you want to read the device
	/// name of an unconnected device, use DeviceSet or just
	/// load the config manually with Barry::ConfigFile.
	std::string GetDeviceName(Barry::Pin pin) const;

	//
	// overrides
	//
	virtual bool OnInit();
	virtual int OnExit();
};

DECLARE_APP(BarryDesktopApp)

#endif

