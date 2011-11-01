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
#include <sys/socket.h>

namespace Barry {

void *callback_wrapper(void *data)
{
	Barry::Thread::CallbackData *cbdata = (Barry::Thread::CallbackData*) data;

	return (*cbdata->callback)(cbdata);
}

Thread::CallbackData::CallbackData(void *userdata)
	: callback(0)
	, stopflag(false)
	, userdata(userdata)
{
}

Thread::Thread(int socket,
		void *(*callback)(Barry::Thread::CallbackData *data),
		void *userdata)
	: m_socket(socket)
{
	m_data = new CallbackData(userdata);
	m_data->callback = callback;

	int ret = pthread_create(&thread, NULL, callback_wrapper, m_data);
	if( ret ) {
		delete m_data;
		throw Barry::ErrnoError("Thread: pthread_create failed.", ret);
	}
}

Thread::~Thread()
{
	if( pthread_join(thread, NULL) == 0 ) {
		// successful join, thread is dead, free to free
		delete m_data;
	}
	else {
		// thread is in la-la land, must leak m_data here for safety
	}
}

void Thread::StopFlag()
{
	// http://stackoverflow.com/questions/2486335/wake-up-thread-blocked-on-accept-call

	// set flag first, before waking thread
	m_data->stopflag = true;

	// shutdown the socket, but leave the fd valid, thereby waking up
	// the thread if it is blocked on accept()
	shutdown(m_socket, SHUT_RDWR);
}

} // namespace Barry

