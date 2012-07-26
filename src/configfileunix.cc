///
/// \file	configfileunix.cc
///		Barry configuration class, for one device PIN, using Unix APIs
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
#include "getpwuid.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>

namespace Barry {


/// Creates a tar.gz filename using PIN + date + time + label.
/// Does not include any path, just returns a new filename.
std::string MakeBackupFilename(const Barry::Pin &pin,
				const std::string &label)
{
	using namespace std;

	time_t t = time(NULL);
	struct tm *lt = localtime(&t);

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
		<< setw(4) << setfill('0') << (lt->tm_year + 1900)
		<< setw(2) << setfill('0') << (lt->tm_mon + 1)
		<< setw(2) << setfill('0') << lt->tm_mday
		<< "-"
		<< setw(2) << setfill('0') << lt->tm_hour
		<< setw(2) << setfill('0') << lt->tm_min
		<< setw(2) << setfill('0') << lt->tm_sec
		<< fileLabel
		<< ".tar.gz";
	return tarfilename.str();
}

void ConfigFile::BuildFilename()
{
	size_t strsize = 255 * 5;
	char *strbuf = new char[strsize];
	struct passwd pwbuf;
	struct passwd *pw;

	getpwuid_r(getuid(), &pwbuf, strbuf, strsize, &pw);
	if( !pw ) {
		delete [] strbuf;
		throw ConfigFileError(_("BuildFilename: getpwuid failed"), errno);
	}

	m_filename = pw->pw_dir;
	m_filename += "/.barry/backup/";
	m_filename += m_pin.Str();
	m_filename += "/config";

	delete [] strbuf;
}

void ConfigFile::BuildDefaultPath()
{
	struct passwd *pw = getpwuid(getuid());
	m_path = pw->pw_dir;
	m_path += "/.barry/backup/";
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

	if( access(path.c_str(), F_OK) == 0 )
		return true;

	std::string base;
	std::string::size_type slash = 0;
	while( (slash = path.find('/', slash + 1)) != std::string::npos ) {
		base = path.substr(0, slash);
		if( access(base.c_str(), F_OK) != 0 ) {
			if( mkdir(base.c_str(), 0755) == -1 ) {
				if( perr ) {
					*perr = _("mkdir() failed to create: ") + base + ": ";
					*perr += strerror(errno);
				}
				return false;
			}
		}
	}
	if( mkdir(path.c_str(), 0755) == -1 ) {
		if( perr ) {
			*perr = _("last mkdir() failed to create: ") + path + ": ";
			*perr += strerror(errno);
		}
		return false;
	}
	return true;
}

void GlobalConfigFile::BuildFilename()
{
	struct passwd *pw = getpwuid(getuid());
	if( !pw )
		throw ConfigFileError(_("BuildFilename: getpwuid failed"), errno);

	m_filename = pw->pw_dir;
	m_filename += "/.barry/config";

	// build the global path too, since this never changes
	m_path = pw->pw_dir;
	m_path += "/.barry";
}

} // namespace Barry

