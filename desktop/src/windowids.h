///
/// \file	windowids.h
///		Window IDs for the Barry Desktop GUI
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
	SyncMode_ConfigureButton,
	SyncMode_RunAppButton,
	SyncMode_1WayResetButton,
	SyncMode_DeviceList,

	// BrowseMode IDs
	BrowseMode_DBDBList,
	BrowseMode_RecordList,
	BrowseMode_ShowAllCheckbox,
	BrowseMode_AddRecordButton,
	BrowseMode_EditRecordButton,
	BrowseMode_DeleteRecordButton,

	// Dialog IDs
	Dialog_GroupCfg,
	Dialog_GroupCfg_EngineCombo,
	Dialog_GroupCfg_AppCombo,
	Dialog_GroupCfg_AppConfigButton,
	Dialog_SyncStatus,
	Dialog_SyncStatus_RunAppButton,
	Dialog_SyncStatus_SyncAgainButton,
	Dialog_SyncStatus_KillCloseButton,
	Dialog_SyncStatus_SyncTerminated,
	Dialog_SyncStatus_ShowDetailsButton,
	Dialog_SyncStatus_Timer,
	Dialog_Conflict,
	Dialog_Conflict_DataList,
	Dialog_Conflict_SelectButton1,
	Dialog_Conflict_SelectButton2,
	Dialog_Conflict_SelectButton3,
	Dialog_Conflict_SelectButton4,
	Dialog_Conflict_SelectButton5,
	Dialog_Conflict_SelectButton6,
	Dialog_Conflict_SelectButton7,
	Dialog_Conflict_SelectButton8,
	Dialog_Conflict_SelectButton9,
	Dialog_Conflict_ShowButton1,
	Dialog_Conflict_ShowButton2,
	Dialog_Conflict_ShowButton3,
	Dialog_Conflict_ShowButton4,
	Dialog_Conflict_ShowButton5,
	Dialog_Conflict_ShowButton6,
	Dialog_Conflict_ShowButton7,
	Dialog_Conflict_ShowButton8,
	Dialog_Conflict_ShowButton9,
	Dialog_Conflict_DuplicateButton,
	Dialog_Conflict_AbortButton,
	Dialog_Conflict_IgnoreButton,
	Dialog_Conflict_KeepNewerButton,
	Dialog_Conflict_KillSyncButton,
	Dialog_Conflict_AlwaysCheckbox,
	Dialog_ContactEdit,

	SysMenu_FirstItem,

	SysMenu_VerboseLogging = SysMenu_FirstItem,
	SysMenu_RenameDevice,
	SysMenu_ResetDevice,
	SysMenu_RescanUsb,

	SysMenu_LastItem = SysMenu_RescanUsb
};

#endif

