///
/// \file	tarfile.h
///		API for reading and writing sequentially from compressed
///		tar files.

/*
    Copyright (C) 2007-2011, Chris Frey <cdfrey@foursquare.net>

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

#ifndef __REUSE_TARFILE_H__
#define __REUSE_TARFILE_H__

#include "dll.h"
#include <string>
#include <stdexcept>
#include <libtar.h>

namespace Barry {
	class Data;
}

namespace reuse {

//
// Compression options... more op sets can be added based on
// threading needs, or threading library support.
//

/// Compression op set for zlib, non-threadsafe.
extern tartype_t gztar_ops_nonthread;

class BXLOCAL TarFile
{
	TAR *m_tar;
	bool m_throw;
	bool m_writemode;
	std::string m_last_error;

private:
	bool False(const char *msg);
	bool False(const std::string &str) { return False(str.c_str()); }
	bool False(const std::string &msg, int err);

public:
	class TarError : public std::runtime_error
	{
	public:
		TarError(const std::string &msg) : std::runtime_error(msg) {}
	};

public:
	explicit TarFile(const char *filename, bool write = false,
		tartype_t *compress_ops = 0, bool always_throw = false);
	~TarFile();

	const std::string& get_last_error() const { return m_last_error; }

	bool Close();

	/// Appends a new file to the current tarfile, using tarpath as
	/// its internal filename, and data as the complete file contents.
	/// Uses current date and time as file mtime.
	bool AppendFile(const char *tarpath, const std::string &data);

	/// Reads next available file into data, filling tarpath with
	/// internal filename from tarball.
	/// Returns false on end of archive.
	bool ReadNextFile(std::string &tarpath, std::string &data);
	bool ReadNextFile(std::string &tarpath, Barry::Data &data);

	/// Read next available filename, skipping the data if it is
	/// a regular file
	bool ReadNextFilenameOnly(std::string &tarpath);
};

}

#endif

