///
/// \file	scoped_lock.h
///		Simple scope class for dealing with pthread mutex locking.
///

/*
    Copyright (C) 2008-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SCOPED_LOCK_H__
#define __BARRY_SCOPED_LOCK_H__

#include <pthread.h>

namespace Barry {

class scoped_lock
{
	pthread_mutex_t *m_mutex;

public:
	scoped_lock(pthread_mutex_t &mutex)
		: m_mutex(&mutex)
	{
		while( pthread_mutex_lock(m_mutex) != 0 )
			;
	}

	~scoped_lock()
	{
		unlock();
	}

	void unlock()
	{
		if( m_mutex ) {
			pthread_mutex_unlock(m_mutex);
			m_mutex = 0;
		}
	}
};

} // namespace Barry

#endif

