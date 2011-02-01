///
/// \file	barrydesktop.cc
///		Program entry point for the desktop GUI
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

#include "barrydesktop.h"
#include "util.h"
#include "osbase.h"
#include "BaseFrame.h"
#include <memory>
#include <wx/splash.h>
#include <wx/mstream.h>

using namespace std;

UsbScanSplash::UsbScanSplash()
{
	wxImage scanpng(GetImageFilename(_T("scanning.png")));
	wxBitmap scanning(scanpng);
	std::auto_ptr<wxSplashScreen> splash( new wxSplashScreen(
		scanning, wxSPLASH_CENTRE_ON_SCREEN, 0,
		NULL, -1, wxDefaultPosition, wxDefaultSize,
		wxSIMPLE_BORDER) );
	splash->Show(true);
	wxGetApp().Yield();
	wxGetApp().Yield();
}

UsbScanSplash::~UsbScanSplash()
{
}

//////////////////////////////////////////////////////////////////////////////
// BarryDesktopApp

BarryDesktopApp::BarryDesktopApp()
	: m_global_config("BarryDesktop")
	, m_set( new OpenSync::APISet )
{
}

void BarryDesktopApp::Probe()
{
	// start fresh
	m_results.clear();

	try {
		// This can throw Usb::Error exceptions
		Barry::Probe probe;
		m_results = probe.GetResults();
	}
	catch( Usb::Error &e ) {
		wxString msg = _T("A serious error occurred while probing the USB subsystem for BlackBerry(R) devices: ");
		msg += wxString(e.what(), wxConvUTF8);
		wxMessageBox(msg, _T("USB Error"), wxOK | wxICON_ERROR);
	}
}

wxBitmap BarryDesktopApp::GetScreenshot(const Barry::ProbeResult &device) const
{
	// FIXME - will need to eventually move the controller object
	// into the main app, I think, and maybe the modes too, so
	// that multiple menu commands can work simultaneously

	Barry::Controller con(device);
	Barry::Mode::JavaLoader javaloader(con);

	javaloader.Open();
	javaloader.StartStream();

	Barry::JLScreenInfo info;
	Barry::Data image;
	javaloader.GetScreenshot(info, image);

	// Convert to BMP format
	Barry::Data bitmap(-1, GetTotalBitmapSize(info));
	Barry::ScreenshotToBitmap(info, image, bitmap);

	// Load as wxImage (sigh)
	wxMemoryInputStream stream(bitmap.GetData(), bitmap.GetSize());
	wxImage bmp(stream, wxBITMAP_TYPE_BMP);
	bmp.Rescale(180, 100, wxIMAGE_QUALITY_HIGH);
	return wxBitmap(bmp);
}

void BarryDesktopApp::SetDeviceName(Barry::Pin pin, const std::string &name)
{
	if( !pin.Valid() )
		return;

	// load device config
	Barry::ConfigFile cfg(pin);

	// re-save device config
	cfg.SetDeviceName(name);
	cfg.Save();

	// update our results if this device exists in our list
	int index = Barry::Probe::Find(m_results, pin);
	if( index != -1 ) {
		m_results[index].m_cfgDeviceName = name;
	}
}

std::string BarryDesktopApp::GetDeviceName(Barry::Pin pin) const
{
	string name;
	int index = Barry::Probe::Find(m_results, pin);
	if( index != -1 )
		name = m_results[index].m_cfgDeviceName;
	return name;
}

void BarryDesktopApp::ShowMissingOpenSyncMessage()
{
	wxMessageBox(_T("No OpenSync libraries were found. Sync will be unavailable until you install OpenSync version 0.22 or version 0.4x on your system, along with the needed plugins."), _T("OpenSync Not Found"), wxOK | wxICON_INFORMATION);
}

bool BarryDesktopApp::OnInit()
{
	// Add a PNG handler for loading buttons and backgrounds
	wxImage::AddHandler( new wxPNGHandler );

	std::auto_ptr<UsbScanSplash> splash( new UsbScanSplash );

	// Initialize Barry and USB
	Barry::Init(m_global_config.VerboseLogging());

	// Scan bus at the beginning so we know what devices we've got
	Probe();

	// Search for available OpenSync libraries
	if( m_set->OpenAvailable() == 0 ) {
		ShowMissingOpenSyncMessage();
	}

	// Create the main frame window where all the action happens
	wxImage back(GetImageFilename(_T("background.png")));
	if( !back.IsOk() ) {
		Yield();
		return false;
	}
	BaseFrame *frame = new BaseFrame(back);

	// Clean up the splash screen, and init the main frame
	splash.reset();
	SetTopWindow(frame);
	frame->Show(true);

	return true;
}

int BarryDesktopApp::OnExit()
{
	try {
		m_global_config.Save();
	}
	catch( std::exception &e ) {
		cerr << "Exception caught while saving config: "
			<< e.what() << endl;
	}

	return 0;
}

// This takes care of main() and wxGetApp() for us.
IMPLEMENT_APP(BarryDesktopApp)

