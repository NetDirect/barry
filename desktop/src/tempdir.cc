///
/// \file	tempdir.cc
///		Temp directory & file wrapper class
///

/*
    Copyright (C) 2009-2012, Chris Frey <cdfrey@foursquare.net>
    The idea to use glib's g_get_tmp_dir() came from opensync's osynctool.

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

#include "tempdir.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <sstream>
#include <stdexcept>

TempDir::TempDir(const char *basename)
	: m_template(0)
	, m_files(0)
{
	m_template = g_strdup_printf("%s/%s-XXXXXX", g_get_tmp_dir(), basename);
	if( mkdtemp(m_template) == NULL ) {
		g_free(m_template);
		throw std::runtime_error(std::string("Cannot create temp directory: ") + strerror(errno));
	}
}

TempDir::~TempDir()
{
	// delete all files
	for( int i = 0; i < m_files; i++ ) {
		unlink(MakeFilename(i).c_str());
	}

	// delete directory
	rmdir(m_template);

	// cleanup memory
	g_free(m_template);
}

std::string TempDir::MakeFilename(int file_id) const
{
	std::ostringstream oss;
	oss << m_template << "/" << file_id;
	return oss.str();
}

std::string TempDir::GetNewFilename()
{
	return MakeFilename(m_files++);
}

