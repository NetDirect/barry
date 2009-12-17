///
/// \file	windowids.h
///		Window IDs for the Barry Desktop GUI
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_WINDOWIDS_H__
#define __BARRYDESKTOP_WINDOWIDS_H__

#include <wx/wx.h>

//////////////////////////////////////////////////////////////////////////////
// IDs for controls and menu items (no menus in this app yet)
enum {
	SysMenu_Exit = wxID_EXIT,
	SysMenu_About = wxID_ABOUT,

	MainMenu_FirstButton = wxID_HIGHEST,

	MainMenu_BackupAndRestore = MainMenu_FirstButton,
	MainMenu_Sync,
	MainMenu_Modem,
	MainMenu_AppLoader,
	MainMenu_DeviceSwitch,
	MainMenu_BrowseDatabases,
	MainMenu_MediaManagement,
	MainMenu_Misc,

	MainMenu_LastButton = MainMenu_Misc,

	// Main menu buttons that don't always exist
	MainMenu_BackButton,

	// Clickable, "hot" images that do something
	HotImage_BarryLogo,
	HotImage_NetDirectLogo,

	// Misc IDs
	Ctrl_DeviceCombo,
	Process_BackupAndRestore,

	// SyncMode IDs
	SyncMode_SyncNowButton,
	SyncMode_DeviceList,

	// Dialog IDs
	Dialog_GroupCfg,
	Dialog_GroupCfg_EngineCombo,
	Dialog_GroupCfg_AppCombo,
	Dialog_GroupCfg_AppConfigButton,

	SysMenu_FirstItem,

	SysMenu_VerboseLogging = SysMenu_FirstItem,
	SysMenu_RescanUsb,

	SysMenu_LastItem = SysMenu_RescanUsb
};

#endif

