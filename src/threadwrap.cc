///
/// \file	threadwrap.cc
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

#include "threadwrap.h"
#include "error.h"


namespace Barry {

Thread::Thread(int socket, void *(*callback)(void *data), void *data)
{
	int ret = pthread_create(&thread, NULL, callback, data);
	if( ret ) {
		throw Barry::ErrnoError("Thread: pthread_create failed.", ret);
	}
}


Thread::~Thread()
{
	pthread_join(thread, NULL);
}


void Thread::Dispose()
{
	int ret = pthread_cancel(thread);
	if( ret ) {
		throw Barry::ErrnoError("Thread: pthread_cancel failed.", ret);
	}
}

} // namespace Barry

