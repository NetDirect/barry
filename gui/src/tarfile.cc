///
/// \file	tarfile.cc
///		API for reading and writing sequentially from compressed
///		tar files.

/*
    Copyright (C) 2007-2008, Chris Frey <cdfrey@foursquare.net>

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

#include "tarfile.h"

#include <fcntl.h>
#include <errno.h>

namespace reuse {

TarFile::TarFile(const char *filename,
		 bool create,
		 tartype_t *compress_ops,
		 bool always_throw)
	: m_tar(0),
	m_throw(always_throw),
	m_writemode(create)
{
	// figure out how to handle the file flags/modes
	int flags = 0;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	if( m_writemode ) {
		flags = O_WRONLY | O_CREAT | O_EXCL;
	}
	else {
		flags = O_RDONLY;
	}

	// open... throw on error, as we are in the constructor
	if( tar_open(&m_tar, const_cast<char*>(filename),
		compress_ops, flags, mode, TAR_VERBOSE | TAR_GNU) == -1 ) {
		throw TarError(std::string("Unable to open tar file: ") + strerror(errno));
	}
}

TarFile::~TarFile()
{
	try {
		Close();
	} catch( TarError &te ) {}
}

bool TarFile::False(const char *msg)
{
	m_last_error = msg;
	if( m_throw )
		throw TarError(msg);
	else
		return false;
}

bool TarFile::False(const std::string &msg, int err)
{
	std::string str = msg;
	str += ": ";
	str += strerror(err);
	return False(str);
}

bool TarFile::Close()
{
	if( m_tar ) {
		if( m_writemode ) {
			if( tar_append_eof(m_tar) != 0 )
				return False("Unable to write eof", errno);
		}

		if( tar_close(m_tar) != 0 ) {
			return False("Unable to close file", errno);
		}
		m_tar = 0;
	}
	return true;
}

/// Appends a new file to the current tarfile, using tarpath as
/// its internal filename, and data as the complete file contents.
/// Uses current date and time as file mtime.
bool TarFile::AppendFile(const char *tarpath, const std::string &data)
{
	// write standard file header
	th_set_type(m_tar, REGTYPE);
	th_set_mode(m_tar, 0644);
	th_set_path(m_tar, const_cast<char*>(tarpath));
	th_set_user(m_tar, 0);
	th_set_group(m_tar, 0);
	th_set_size(m_tar, data.size());
	th_set_mtime(m_tar, time(NULL));
	if( th_write(m_tar) != 0 ) {
		return False("Unable to write tar header", errno);
	}

	// write the data in blocks until finished
	char block[T_BLOCKSIZE];
	for( size_t pos = 0; pos < data.size(); pos += T_BLOCKSIZE ) {
		memset(block, 0, T_BLOCKSIZE);

		size_t size = T_BLOCKSIZE;
		if( data.size() - pos < T_BLOCKSIZE )
			size = data.size() - pos;

		memcpy(block, data.data() + pos, size);

		if( tar_block_write(m_tar, block) != T_BLOCKSIZE ) {
			return False("Unable to write block", errno);
		}
	}

	return true;
}

/// Reads next available file into data, filling tarpath with
/// internal filename from tarball.
bool TarFile::ReadNextFile(std::string &tarpath, std::string &data)
{
	// start fresh
	tarpath.clear();
	data.clear();

	// read next tar file header
	if( th_read(m_tar) != 0 ) {
		// this is not necessarily an error, as it could just
		// be the end of file, so a simple false is good here,
		// don't throw an exception
		m_last_error = "";
		return false;
	}

	// write standard file header
	if( !TH_ISREG(m_tar) ) {
		return False("Only regular files are supported inside a tarball.");
	}

	tarpath = th_get_pathname(m_tar);
	size_t size = th_get_size(m_tar);

	// read the data in blocks until finished
	char block[T_BLOCKSIZE];
	for( size_t pos = 0; pos < size; pos += T_BLOCKSIZE ) {
		memset(block, 0, T_BLOCKSIZE);

		size_t readsize = T_BLOCKSIZE;
		if( size - pos < T_BLOCKSIZE )
			readsize = size - pos;

		if( tar_block_read(m_tar, block) != T_BLOCKSIZE ) {
			return False("Unable to read block", errno);
		}

		data.append(block, readsize);
	}

	return true;
}

/// Read next available filename, skipping the data if it is
/// a regular file
bool TarFile::ReadNextFilenameOnly(std::string &tarpath)
{
	// start fresh
	tarpath.clear();

	// read next tar file header
	if( th_read(m_tar) != 0 ) {
		// this is not necessarily an error, as it could just
		// be the end of file, so a simple false is good here,
		// don't throw an exception
		m_last_error = "";
		return false;
	}

	// write standard file header
	if( !TH_ISREG(m_tar) ) {
		return False("Only regular files are supported inside a tarball.");
	}

	tarpath = th_get_pathname(m_tar);

	if( tar_skip_regfile(m_tar) != 0 ) {
		return False("Unable to skip tar file", errno);
	}

	return true;
}


} // namespace reuse


#ifdef __TEST_MODE__

#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
	try {
		cout << "Writing test file..." << endl;
		reuse::TarFile output("tartest.tar.gz", true, true, true);
		std::string data;
		for( int i = 0; i < 60; i++ ) {
			data.append("0123456789", 10);
		}

		output.AppendFile("path1/test1.txt", data);
		output.AppendFile("path2/test2.txt", data);
		output.Close();


		cout << "Reading test file..." << endl;
		reuse::TarFile input("tartest.tar.gz", false, true, true);
		std::string path, incoming;

		while( input.ReadNextFile(path, incoming) ) {
			cout << "Read: " << path
			     << " Data: "
			     << (( data == incoming ) ? "equal" : "different")
			     << endl;
		}

		input.Close();

		unlink("tartest.tar.gz");

	} catch( reuse::TarFile::TarError &te ) {
		cerr << te.what() << endl;
		return 1;
	}
}

#endif

