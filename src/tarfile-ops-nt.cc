///
/// \file	tarfile-ops-nt.cc
///		Non-thread safe operation functions for a libtar-compatible
///		zlib compression interface.

/*
    Copyright (C) 2007-2012, Chris Frey <cdfrey@foursquare.net>

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <zlib.h>

#include <assert.h>

namespace reuse {

namespace gztar_nonthread {

	namespace {
		// array of compressed file handles... needed for architectures
		// where sizeof(int) != sizeof(gzFile)
		gzFile *gzHandles = 0;
		unsigned int gzArraySize = 0;
	}

	int open_compressed(const char *file, int flags, mode_t mode)
	{
		unsigned int index = 0;
		for( ; index < gzArraySize; index++ ) {
			if( gzHandles[index] == 0 )
				break;
		}
		if( index >= gzArraySize ) {
			gzFile *h = (gzFile*) realloc(gzHandles,
				(gzArraySize + 100) * sizeof(gzFile));
			if( h ) {
				gzHandles = h;
				gzArraySize += 100;
			}
			else {
				return -1;
			}
		}

		int fd = open(file, flags, mode);
		if( fd == -1 )
			return -1;

		gzFile gfd = gzdopen(fd, (flags & O_WRONLY) ? "wb9" : "rb");
		if( gfd == NULL ) {
			close(fd);
			return -1;
		}

		gzHandles[index] = gfd;
		return index;
	}

	int close_compressed(int fd)
	{
		unsigned int ufd = fd;
		assert( ufd < gzArraySize );
		int ret = gzclose(gzHandles[ufd]);
		gzHandles[ufd] = 0;
		return ret;
	}

	ssize_t read_compressed(int fd, void *buf, size_t size)
	{
		unsigned int ufd = fd;
		assert( ufd < gzArraySize );
		return gzread(gzHandles[ufd], buf, size);
	}

	ssize_t write_compressed(int fd, const void *buf, size_t size)
	{
		unsigned int ufd = fd;
		assert( ufd < gzArraySize );
		return gzwrite(gzHandles[ufd], buf, size);
	}

} // namespace gztar_nonthread


tartype_t gztar_ops_nonthread = {
	(openfunc_t) gztar_nonthread::open_compressed,
	gztar_nonthread::close_compressed,
	gztar_nonthread::read_compressed,
	gztar_nonthread::write_compressed
};


} // namespace reuse

