///
/// \file	configfilewin32.cc
///		Barry configuration class, for one device PIN, using Win32 APIs
///

/*
    Copyright (C) 2007-2012, Net Direct Inc. (http://www.netdirect.ca/)
	Portions Copyright (C) 2012, RealVNC Ltd.

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

#include "i18n.h"
#include "configfile.h"
#include "error.h"
#include "r_message.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>
#include <Shlobj.h>
#include <winbase.h>

// Returned by GetFileAttributes to indicate failure
#define INVALID_ATTRIB 0xFFFFFFFF

namespace Barry {


/// Creates a tar.gz filename using PIN + date + time + label.
/// Does not include any path, just returns a new filename.
std::string MakeBackupFilename(const Barry::Pin &pin,
				const std::string &label)
{
	using namespace std;

	SYSTEMTIME st;
	GetLocalTime(&st);

	std::string fileLabel = label;
	if( fileLabel.size() ) {
		// prepend a hyphen
		fileLabel.insert(fileLabel.begin(), '-');

		// translate all spaces and slashes
		for( size_t i = 0; i < fileLabel.size(); i++ ) {
			if( fileLabel[i] == ' ' )
				fileLabel[i] = '_';
			else if( fileLabel[i] == '/' )
				fileLabel[i] = '-';
			else if( fileLabel[i] == '\\' )
				fileLabel[i] = '-';
		}
	}

	ostringstream tarfilename;
	tarfilename << pin.Str() << "-"
		<< setw(4) << setfill('0') << st.wYear
		<< setw(2) << setfill('0') << st.wMonth
		<< setw(2) << setfill('0') << st.wDay
		<< "-"
		<< setw(2) << setfill('0') << st.wHour
		<< setw(2) << setfill('0') << st.wMinute
		<< setw(2) << setfill('0') << st.wSecond
		<< fileLabel
		<< ".tar.gz";
	return tarfilename.str();
}

void ConfigFile::BuildFilename()
{
	TCHAR dirName[MAX_PATH];
	CHAR dirNameA[MAX_PATH];
	if( !SHGetSpecialFolderPath(NULL, dirName, CSIDL_APPDATA, TRUE) ) {
		throw ConfigFileError(_("BuildFilename: SHGetSpecialFolderPath failed"), GetLastError());
	}
	if( WideCharToMultiByte(CP_ACP, 0, dirName, -1, dirNameA, MAX_PATH, NULL, NULL) <= 0 ) {
		throw ConfigFileError(_("BuildFilename: conversion failed"), GetLastError());
	}
	dirNameA[MAX_PATH-1] = 0; // Make sure it's NUL terminated
	m_filename = dirNameA;
	m_filename += "\\.barry\\backup\\";
	m_filename += m_pin.Str();
	m_filename += "\\config";
}

void ConfigFile::BuildDefaultPath()
{
	TCHAR dirName[MAX_PATH];
	CHAR dirNameA[MAX_PATH];
	if( !SHGetSpecialFolderPath(NULL, dirName, CSIDL_APPDATA, TRUE) ) {
		throw ConfigFileError(_("BuildDefaultPath: SHGetSpecialFolderPath failed"), GetLastError());
	}
	if( WideCharToMultiByte(CP_ACP, 0, dirName, -1, dirNameA, MAX_PATH, NULL, NULL) <= 0 ) {
		throw ConfigFileError(_("BuildFilename: conversion failed"), GetLastError());
	}
	dirNameA[MAX_PATH-1] = 0; // Make sure it's NUL terminated
	m_path = dirNameA;
	m_path += "\\.barry\\backup\\";
	m_path += m_pin.Str();
}

/// Checks that the path in path exists, and if not, creates it.
/// Returns false if unable to create path, true if ok.
bool ConfigFile::CheckPath(const std::string &path, std::string *perr)
{
	if( path.size() == 0 ) {
		if( perr )
			*perr = _("ConfigFile::CheckPath(): path is empty!");
		return false;
	}

	TCHAR dirName[MAX_PATH];
	if( MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, dirName, MAX_PATH) <= 0 ) {
		if( perr )
			*perr = _("Failed to convert to widechar");
		return false;
	}
	dirName[MAX_PATH-1] = 0; // Make sure it's NUL terminated

	DWORD attrs = GetFileAttributes(dirName);
	if( (attrs != INVALID_ATTRIB) &&
		((attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) )
		return true;


	for( int i = 0; dirName[i] != 0; ++i ) {
		if( dirName[i] != '\\' )
			continue;
		// At a path separator, turn it into NUL so
		// that string handling APIs see just this part of the path
		dirName[i] = 0;
		DWORD attrs = GetFileAttributes(dirName);
		if( (attrs != INVALID_ATTRIB) &&
			((attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) )
			continue;
		if( !CreateDirectory(dirName, NULL) ) {
				if( perr ) {
					*perr = _("failed to create directory");
				}
				return false;
		}
		// Turn it back into a directory separator
		dirName[i] = '\\';
	}
	return true;
}

void GlobalConfigFile::BuildFilename()
{
	TCHAR dirName[MAX_PATH];
	CHAR dirNameA[MAX_PATH];
	if (!SHGetSpecialFolderPath(NULL, dirName, CSIDL_APPDATA, TRUE)) {
		throw ConfigFileError(_("BuildFilename: SHGetSpecialFolderPath failed"), GetLastError());
	}
	if (WideCharToMultiByte(CP_ACP, 0, dirName, -1, dirNameA, MAX_PATH, NULL, NULL) <= 0) {
		throw ConfigFileError(_("BuildFilename: conversion failed"), GetLastError());
	}
	dirNameA[MAX_PATH-1] = 0; // Make sure it's NUL terminated
	m_filename = dirNameA;
	m_filename += "\\.barry\\config";

	// build the global path too, since this never changes
	m_path = dirNameA;
	m_path += "\\.barry";
}

} // namespace Barry

