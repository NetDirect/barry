///
/// \file	dataqueue.cc
///		FIFO queue of Data objects
///

/*
    Copyright (C) 2008-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "dataqueue.h"
#include "scoped_lock.h"
#include "data.h"
#include "time.h"

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// DataQueue class

DataQueue::DataQueue()
{
	pthread_mutex_init(&m_waitMutex, NULL);
	pthread_cond_init(&m_waitCond, NULL);

	pthread_mutex_init(&m_accessMutex, NULL);
}

DataQueue::~DataQueue()
{
	scoped_lock lock(m_accessMutex);	// FIXME - is this sane?

	while( m_queue.size() ) {
		delete m_queue.front();
		m_queue.pop();
	}
}

//
// push
//
/// Pushes data into the end of the queue.
///
/// The queue owns this pointer as soon as the function is
/// called.  In the case of an exception, it will be freed.
/// Performs a thread broadcast once new data has been added.
///
void DataQueue::push(Data *data)
{
	try {

		{
			scoped_lock lock(m_accessMutex);
			m_queue.push(data);
		}

		scoped_lock wait(m_waitMutex);
		pthread_cond_broadcast(&m_waitCond);

	}
	catch(...) {
		delete data;
		throw;
	}
}

//
// pop
//
/// Pops the next element off the front of the queue.
///
/// Returns 0 if empty.
/// The queue no longer owns this pointer upon return.
///
Data* DataQueue::pop()
{
	scoped_lock lock(m_accessMutex);

	if( m_queue.size() == 0 )
		return 0;

	Data *ret = m_queue.front();
	m_queue.pop();
	return ret;
}

//
// wait_pop
//
/// Pops the next element off the front of the queue, and
/// waits until one exists if empty.  If still no data
/// on timeout, returns null.
/// (unlock the access mutex while waiting!)
///
/// Timeout specified in milliseconds.  Default is wait forever.
///
Data* DataQueue::wait_pop(int timeout)
{
	Data *ret = 0;

	// check if something's there already
	{
		scoped_lock access(m_accessMutex);
		if( m_queue.size() ) {
			ret = m_queue.front();
			m_queue.pop();
			return ret;
		}
	}

	// nothing there, so wait...

	if( timeout == -1 ) {
		// no timeout
		int size = 0;
		do {
			{
				scoped_lock wait(m_waitMutex);
				pthread_cond_wait(&m_waitCond, &m_waitMutex);
			}

			// anything there?
			scoped_lock access(m_accessMutex);
			size = m_queue.size();
			if( size != 0 ) {
				// already have the lock, return now
				ret = m_queue.front();
				m_queue.pop();
				return ret;
			}

		} while( size == 0 );
	}
	else {
		// timeout in conditional wait
		struct timespec to;
		scoped_lock wait(m_waitMutex);
		pthread_cond_timedwait(&m_waitCond, &m_waitMutex,
			ThreadTimeout(timeout, &to));
	}

	scoped_lock access(m_accessMutex);
	if( m_queue.size() == 0 )
		return 0;

	ret = m_queue.front();
	m_queue.pop();
	return ret;
}

//
// append_from
//
/// Pops all data from other and appends it to this.
///
/// After calling this function, other will be empty, and
/// this will contain all its data.
///
/// In the case of an exception, any uncopied data will
/// remain in other.
///
/// This is a locking optimization, so all copying can happen
/// inside one lock, instead of locking for each copy.
///
void DataQueue::append_from(DataQueue &other)
{
	scoped_lock us(m_accessMutex);
	scoped_lock them(other.m_accessMutex);

	while( other.m_queue.size() ) {
		m_queue.push( other.m_queue.front() );

		// only pop after the copy, since in the
		// case of an exception we want to leave other intact
		other.m_queue.pop();
	}
}

//
// empty
//
/// Returns true if the queue is empty.
///
bool DataQueue::empty() const
{
	scoped_lock access(m_accessMutex);
	return m_queue.size() == 0;
}

//
// size
//
/// Returns number of items in the queue.
///
size_t DataQueue::size() const
{
	scoped_lock access(m_accessMutex);
	return m_queue.size();
}

} // namespace Barry


#ifdef __DQ_TEST_MODE__

#include <iostream>

using namespace std;
using namespace Barry;

void *WriteThread(void *userdata)
{
	DataQueue *dq = (DataQueue*) userdata;

	dq->push( new Data );
	dq->push( new Data );
	sleep(5);
	dq->push( new Data );

	return 0;
}

void *ReadThread(void *userdata)
{
	DataQueue *dq = (DataQueue*) userdata;

	sleep(1);
	if( Data *d = dq->pop() ) {
		cout << "Received via pop: " << d << endl;
		delete d;
	}
	else {
		cout << "No data in the queue yet." << endl;
	}

	while( Data *d = dq->wait_pop(5010) ) {
		cout << "Received: " << d << endl;
		delete d;
	}
	return 0;
}

int main()
{
	DataQueue from;
	from.push( new Data );

	DataQueue dq;
	dq.append_from(from);

	pthread_t t1, t2;
	pthread_create(&t1, NULL, &ReadThread, &dq);
	pthread_create(&t2, NULL, &WriteThread, &dq);

	pthread_join(t2, NULL);
	pthread_join(t1, NULL);
}

#endif

