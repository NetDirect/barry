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

#include "dll.h"
#include <pthread.h>

namespace Barry {

class BXEXPORT Thread
{
public:
	struct CallbackData
	{
		void *(*callback)(Barry::Thread::CallbackData *data);

		// user-accessible data
		volatile bool stopflag;
		void *userdata;

		explicit CallbackData(void *userdata);
	};

private:
	pthread_t thread;
	int m_socket;
	CallbackData *m_data;

public:
	// Note: the data pointer passed into callback is not the same
	// as userdata.  The data pointer is a pointer to CallbackData
	// which will contain the userdata pointer.
	Thread(int socket,
		void *(*callback)(Barry::Thread::CallbackData *data),
		void *userdata);
	~Thread();

	/// Sets stopflag in the callback data to true
	void StopFlag();
};

} // namespace Barry

#endif

