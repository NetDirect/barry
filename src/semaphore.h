///
/// \file	semaphore.h
///		Simple class implementing a semaphore using pthreads mutex and condvar.
///

/*
    Copyright (C) 2010, RealVNC Ltd.

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

#ifndef __BARRY_SEMAPHORE_H__
#define __BARRY_SEMAPHORE_H__

#include "scoped_lock.h"
#include <pthread.h>

namespace Barry {

class semaphore
{
	pthread_mutex_t *m_mutex;
	pthread_cond_t *m_cv;
	int m_value;
public:
	semaphore(pthread_mutex_t &mutex, pthread_cond_t &cv, int value = 0)
		: m_mutex(&mutex)
		, m_cv(&cv)
		, m_value(value)
	{
	}

	// Waits for the value of this semaphore to be greater than 0 and then
	// decrements it by one before returning.
	void WaitForSignal()
	{
		scoped_lock lock(*m_mutex);
		while( m_value <= 0 ) {
			int ret = pthread_cond_wait(m_cv, m_mutex);
			if( ret != 0 ) {
				throw Barry::Error("semaphore: failed to wait on condvar");
			}
		}
		--m_value;
		lock.unlock();
	}

	// Checks for a semaphore signal without blocking. Returns true and decrements
	// the semaphore if the value is greater than 0, otherwise returns false.
	bool ReceiveSignal()
	{
		bool ret = false;
		scoped_lock lock(*m_mutex);
		if( m_value > 0 ) {
			--m_value;
			ret = true;
		}
		lock.unlock();
		return ret;
	}

	// Increments the value of this semaphore by 1, waking any sleeping threads waiting
	// on this semaphore.
	void Signal()
	{
		scoped_lock lock(*m_mutex);
		++m_value;
		int ret = pthread_cond_signal(m_cv);
		if( ret != 0 ) {
			throw Barry::Error("Condvar: failed to signal condvar");
		}
		lock.unlock();
	}
};

} // namespace Barry

#endif

