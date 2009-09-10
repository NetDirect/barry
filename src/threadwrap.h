///
/// \file	threadwrap.h
///		RAII Wrapper for a single thread.
///

/*
    Copyright (C) 2009, Nicolas VIVIEN

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

#ifndef __BARRY_THREADWRAP_H__
#define __BARRY_THREADWRAP_H__

#include <pthread.h>

namespace Barry {

class Thread
{
private:
	pthread_t thread;

public:
	Thread(int socket, void *(*callback)(void *data), void *data);
	~Thread();

	void dispose();
};

} // namespace Barry

#endif

