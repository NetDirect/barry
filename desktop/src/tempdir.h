///
/// \file	tempdir.h
///		Temp directory & file wrapper class
///

/*
    Copyright (C) 2009-2010, Chris Frey <cdfrey@foursquare.net>

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

#ifndef __BARRYDESKTOP_TEMPDIR_H__
#define __BARRYDESKTOP_TEMPDIR_H__

#include <string>

//
// class TempDir
//
/// This class creates a new temp directory on instantiation,
/// and provides access members to retrieve the directory name
/// and incrementing filenames.  On destruction, all the returned
/// filenames are deleted, and the directory is removed.
///
class TempDir
{
	char *m_template;
	int m_files;

	std::string MakeFilename(int file_id) const;

public:
	TempDir(const char *basename);
	~TempDir();

	std::string GetDir() const { return m_template; }

	/// Returns unique filename in the form of
	/// /tmp/opensyncapi-XXXXX/0, 1, 2, etc.
	std::string GetNewFilename();
};

#endif

